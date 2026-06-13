import asyncio
import threading
import queue
import tkinter as tk
from tkinter import ttk
import json
from datetime import datetime

from bleak import BleakClient, BleakScanner

# ── UUIDs  ──────────────────────────────────
SERVICE_UUID   = "76c3e5d6-ee9e-4f48-bb4d-287d88f872d6"
CHAR_SSID_UUID = "6fc254b7-6bc0-475b-b836-720d2287922d"   # wifi_ssi_uuid
CHAR_PW_UUID   = "cfedbbc2-0598-4581-b1fd-b7d54fec0396"   # wifi_pw_uuid
CHAR_CMD_UUID  = "9823ae51-1729-4f51-b847-112233445566"   # prov_cmd_uuid
CHAR_SENSOR_DATA_UUID = "6a1b2c3d-4e5f-6a7b-8c9d-0e1f2a3b4c5d" # data_val_handle
CHAR_SENSOR_CONFIG_UUID = "b1c2d3e4-f5a6-b7c8-d9e0-f1a2b3c4d5e6" # config_val_handle

_STOP_SENTINEL = None 


class BLEGuiApp:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("ESP32 BLE WiFi Provisioning")
        self.root.geometry("650x520")
        self.root.minsize(650, 520)
        self.root.resizable(False, False)

        self.client: BleakClient | None = None
        self.connected      = False
        self.is_connecting  = False
        self.manual_disconnect = False
        self.scanned_devices: dict = {}
        self.watchdog_timer = None

        # ── Logic phân cấp Device Logs ────────────────
        self.log_buffer = [] 
        self.level_map = {
            "ALL": 0,         
            "VERBOSE": 1,
            "DEBUG": 2,
            "INFO": 3,
            "WARNING": 4,
            "ERROR": 5,
            "NONE": 6         
        }
        self.current_log_level = 0  # Mặc định là ALL

        # ── asyncio event loop ─────────
        self.ble_loop = asyncio.new_event_loop()
        threading.Thread(target=self._run_ble_loop, daemon=True).start()

        # ── Send queue + worker thread ────────────────────────────────────
        self.send_queue: queue.Queue = queue.Queue()
        threading.Thread(
            target=self._send_worker, daemon=True, name="BLE-SendWorker"
        ).start()

        self._build_ui()

    # ── UI ───────────────────────────────────────────────────────────────────
    def _build_ui(self):
        self.notebook = ttk.Notebook(self.root)
        self.notebook.pack(fill="both", expand=True, padx=5, pady=5)

        self.tab_provisioning = ttk.Frame(self.notebook, padding=12)
        self.tab_sensors = ttk.Frame(self.notebook, padding=12)
        self.tab_ota = ttk.Frame(self.notebook, padding=12)
        self.tab_logs = ttk.Frame(self.notebook, padding=12)

        self.notebook.add(self.tab_provisioning, text="Provisioning")
        self.notebook.add(self.tab_sensors, text="Sensors")
        self.notebook.add(self.tab_ota, text="OTA Update")
        self.notebook.add(self.tab_logs, text="Device Logs")

        # ==============================================================
        # TAB 1: PROVISIONING
        # ==============================================================
        main = self.tab_provisioning 

        vcmd32 = (self.root.register(lambda v: len(v) <= 32), '%P')
        vcmd64 = (self.root.register(lambda v: len(v) <= 64), '%P')

        ttk.Label(main, text="Select Device :").place(x=0, y=0)
        self.device_selector = ttk.Combobox(main, state="readonly", width=48)
        self.device_selector.place(x=115, y=0)
        self.device_selector.set("Click 'Scan' to find devices...")
        ttk.Button(main, text="Scan", command=self.scan_device).place(x=460, y=-2)
        
        self.toggle_conn_btn = ttk.Button(main, text="Connect", command=self.connect_device)
        self.toggle_conn_btn.place(x=550, y=-2)

        ttk.Label(main, text="SSID (max 32):").place(x=0, y=34)
        self.ssid_var = tk.StringVar()
        ttk.Entry(main, textvariable=self.ssid_var, width=46,
                  validate="key", validatecommand=vcmd32).place(x=115, y=34)

        ttk.Label(main, text="Password (8-64):").place(x=0, y=64)
        self.pw_var = tk.StringVar()
        ttk.Entry(main, textvariable=self.pw_var, width=46,
                  validate="key", validatecommand=vcmd64).place(x=115, y=64)

        self.send_wifi_btn = ttk.Button(
            main, text="Send Wi-Fi", command=self.enqueue_credentials, state="disabled", width=12
        )
        self.send_wifi_btn.place(x=450, y=62)

        self.trigger_conn_btn = ttk.Button(
            main, text="Connect", command=self.trigger_wifi_connection, state="disabled", width=10
        )
        self.trigger_conn_btn.place(x=550, y=62)

        ttk.Label(main, text="Queue:").place(x=0, y=94)
        self.queue_var = tk.StringVar(value="Queue empty")
        ttk.Label(main, textvariable=self.queue_var, foreground="gray").place(x=115, y=94)

        # -- Trạng thái tổng hợp (BLE, Wi-Fi, Internet) --
        ttk.Label(main, text="BLE Status:").place(x=0, y=124)
        self.status_var = tk.StringVar(value="DISCONNECTED")
        self.status_label = ttk.Label(
            main, textvariable=self.status_var,
            font=("Arial", 10, "bold"), foreground="red"
        )
        self.status_label.place(x=70, y=124)

        ttk.Label(main, text="Wi-Fi:").place(x=220, y=124)
        self.sensor_wifi_var = tk.StringVar(value="--")
        ttk.Label(main, textvariable=self.sensor_wifi_var, font=("Arial", 10, "bold"), foreground="#007ACC").place(x=260, y=124)

        ttk.Label(main, text="Internet:").place(x=400, y=124)
        self.sensor_net_var = tk.StringVar(value="--")
        ttk.Label(main, textvariable=self.sensor_net_var, font=("Arial", 10, "bold"), foreground="#007ACC").place(x=455, y=124)

        self.log_text = tk.Text(main, height=14, width=75) 
        self.log_text.tag_config("error",   foreground="red")
        self.log_text.tag_config("success", foreground="green")
        self.log_text.tag_config("info",    foreground="blue")
        self.log_text.tag_config("queue",   foreground="purple")
        self.log_text.place(x=0, y=155)

        # ==============================================================
        # TAB 2: SENSORS 
        # ==============================================================
        ttk.Label(self.tab_sensors, text="Sensors Data Monitor", font=("Arial", 14, "bold")).pack(pady=20)

        # -- KHUNG HIỂN THỊ DỮ LIỆU LIVE (Nhanh, to, rõ ràng) --
        live_frame = ttk.LabelFrame(self.tab_sensors, text=" Live Sensor Data ", padding=15)
        live_frame.pack(fill="x", padx=40, pady=10)

        self.sensor_time_var = tk.StringVar(value="--:-- --")
        self.sensor_date_var = tk.StringVar(value="--/--/----")
        self.sensor_temp_var = tk.StringVar(value="-- °C")

        ttk.Label(live_frame, text="Current Time:").grid(row=0, column=0, sticky="w", pady=2)
        ttk.Label(live_frame, textvariable=self.sensor_time_var, font=("Arial", 10, "bold"), foreground="#007ACC").grid(row=0, column=1, sticky="w", padx=10, pady=2)

        ttk.Label(live_frame, text="Current Date:").grid(row=1, column=0, sticky="w", pady=2)
        ttk.Label(live_frame, textvariable=self.sensor_date_var, font=("Arial", 10, "bold"), foreground="#007ACC").grid(row=1, column=1, sticky="w", padx=10, pady=2)

        ttk.Label(live_frame, text="Temperature:").grid(row=2, column=0, sticky="w", pady=2)
        ttk.Label(live_frame, textvariable=self.sensor_temp_var, font=("Arial", 10, "bold"), foreground="#D32F2F").grid(row=2, column=1, sticky="w", padx=10, pady=2)

        # -- KHUNG CÀI ĐẶT CẤU HÌNH (Set Time/Alarm giống điện thoại) --
        cfg_frame = ttk.LabelFrame(self.tab_sensors, text=" Device Configuration (RTC) ", padding=15)
        cfg_frame.pack(fill="x", padx=40, pady=10)

        # -- Hàng 0: Nhập Ngày/Tháng/Năm --
        ttk.Label(cfg_frame, text="Date (YYYY-MM-DD):", font=("Arial", 11)).grid(row=0, column=0, sticky="w", pady=(10, 5))
        date_spin_frame = ttk.Frame(cfg_frame)
        date_spin_frame.grid(row=0, column=1, padx=10, sticky="w", pady=(10, 5))

        self.cfg_year_var = tk.StringVar(value="2024")
        self.cfg_month_var = tk.StringVar(value="01")
        self.cfg_date_var = tk.StringVar(value="01")

        tk.Spinbox(date_spin_frame, from_=2000, to=2099, textvariable=self.cfg_year_var, width=5, font=("Consolas", 12), format="%04.0f").pack(side="left")
        ttk.Label(date_spin_frame, text="-", font=("Consolas", 12, "bold")).pack(side="left", padx=2)
        tk.Spinbox(date_spin_frame, from_=1, to=12, textvariable=self.cfg_month_var, width=3, font=("Consolas", 12), format="%02.0f").pack(side="left")
        ttk.Label(date_spin_frame, text="-", font=("Consolas", 12, "bold")).pack(side="left", padx=2)
        tk.Spinbox(date_spin_frame, from_=1, to=31, textvariable=self.cfg_date_var, width=3, font=("Consolas", 12), format="%02.0f").pack(side="left")

        # -- Hàng 1: Nhập Giờ/Phút/Giây --
        ttk.Label(cfg_frame, text="Time (HH:MM:SS):", font=("Arial", 11)).grid(row=1, column=0, sticky="w", pady=(0, 10))
        time_spin_frame = ttk.Frame(cfg_frame)
        time_spin_frame.grid(row=1, column=1, padx=10, sticky="w", pady=(0, 10))
        
        self.cfg_hour_var = tk.StringVar(value="12")
        self.cfg_min_var = tk.StringVar(value="00")
        self.cfg_sec_var = tk.StringVar(value="00")

        tk.Spinbox(time_spin_frame, from_=0, to=23, textvariable=self.cfg_hour_var, width=3, font=("Consolas", 12), format="%02.0f").pack(side="left")
        ttk.Label(time_spin_frame, text=":", font=("Consolas", 12, "bold")).pack(side="left", padx=2)
        tk.Spinbox(time_spin_frame, from_=0, to=59, textvariable=self.cfg_min_var, width=3, font=("Consolas", 12), format="%02.0f").pack(side="left")
        ttk.Label(time_spin_frame, text=":", font=("Consolas", 12, "bold")).pack(side="left", padx=2)
        tk.Spinbox(time_spin_frame, from_=0, to=59, textvariable=self.cfg_sec_var, width=3, font=("Consolas", 12), format="%02.0f").pack(side="left")

        self.btn_set_sensor = ttk.Button(cfg_frame, text="Apply", command=self.send_sensor_config, state="disabled")
        self.btn_set_sensor.grid(row=0, column=2, rowspan=2, padx=15, sticky="ns", pady=10)

        # ==============================================================
        # TAB 3: OTA UPDATE 
        # ==============================================================
        ttk.Label(self.tab_ota, text="Firmware OTA Update", font=("Arial", 14, "bold")).pack(pady=20)
        ttk.Button(self.tab_ota, text="Select .bin File").pack(pady=10)
        ttk.Progressbar(self.tab_ota, orient="horizontal", length=300, mode="determinate").pack(pady=10)

        # ==============================================================
        # TAB 4: DEVICE LOGS 
        # ==============================================================
        log_ctrl_frame = ttk.Frame(self.tab_logs)
        log_ctrl_frame.pack(fill="x", padx=10, pady=10)

        ttk.Label(log_ctrl_frame, text="Level:").pack(side="left", padx=(0, 5))
        
        self.log_level_cb = ttk.Combobox(log_ctrl_frame, state="readonly", width=45)
        self.log_level_cb['values'] = ("ALL", "VERBOSE", "DEBUG", "INFO", "WARNING", "ERROR", "NONE")
        self.log_level_cb.current(0) 
        self.log_level_cb.pack(side="left", padx=(0, 10))

        self.log_level_cb.bind("<<ComboboxSelected>>", lambda e: self._set_log_level())

        self.set_level_btn = ttk.Button(log_ctrl_frame, text="Set", width=6, command=self._set_log_level)
        self.set_level_btn.pack(side="left", padx=5)

        self.clear_log_btn = ttk.Button(log_ctrl_frame, text="Clear", width=6, command=self._clear_device_logs)
        self.clear_log_btn.pack(side="left", padx=5)

        self.device_log_text = tk.Text(
            self.tab_logs, height=20, width=80, 
            bg="white", fg="black", font=("Consolas", 10)
        )
        self.device_log_text.pack(padx=10, pady=(0, 10))
        
        self.device_log_text.tag_config("I", foreground="darkgreen")  
        self.device_log_text.tag_config("D", foreground="blue")       
        self.device_log_text.tag_config("W", foreground="#d97706")    
        self.device_log_text.tag_config("E", foreground="red")        
        self.device_log_text.tag_config("V", foreground="gray")       


    # ── Logging (Provisioning) ──────────────────────────────────────────────
    def _log(self, text: str, tag: str | None = None):
        def _append():
            self.log_text.insert("end", f"> {text}\n", tag)
            self.log_text.see("end")
        self.root.after(0, _append)

    def _update_queue_label(self):
        size = self.send_queue.qsize()
        text = f"{size} item(s) pending" if size else "Queue empty"
        self.root.after(0, lambda: self.queue_var.set(text))

    # ── Logic Device Logs (LỌC CHÍNH XÁC - EXACT MATCH) ─────────────────────
    def _set_log_level(self):
        selected_level = self.log_level_cb.get()
        self.current_log_level = self.level_map.get(selected_level, 0)
        self._refresh_device_logs() 

    def add_device_log(self, level_str: str, tag: str, msg: str):
        lvl_val = self.level_map.get(level_str, 1)
        self.log_buffer.append((lvl_val, tag, msg))
        
        # CHỈ HIỂN THỊ NẾU CHỌN 'ALL' (0) HOẶC TRÙNG KHỚP CHÍNH XÁC
        if self.current_log_level == 0 or lvl_val == self.current_log_level:
            self.device_log_text.insert("end", msg + "\n", tag)
            self.device_log_text.see("end")

    def _refresh_device_logs(self):
        self.device_log_text.delete("1.0", "end")
        for lvl_val, tag, msg in self.log_buffer:
            # KIỂM TRA TRÙNG KHỚP CHÍNH XÁC KHI VẼ LẠI
            if self.current_log_level == 0 or lvl_val == self.current_log_level: 
                self.device_log_text.insert("end", msg + "\n", tag)
        self.device_log_text.see("end")

    def _clear_device_logs(self):
        self.log_buffer.clear()
        self.device_log_text.delete("1.0", "end")




    # ── asyncio loop ─────────────────────────────────────────────────────────
    def _run_ble_loop(self):
        asyncio.set_event_loop(self.ble_loop)
        self.ble_loop.run_forever()

    def _run_async(self, coro):
        return asyncio.run_coroutine_threadsafe(coro, self.ble_loop)

    # ── Send worker ──────────────────────────────────────────────────────────
    def _send_worker(self):
        while True:
            item = self.send_queue.get()          

            if item is _STOP_SENTINEL:
                self.send_queue.task_done()
                break

            char_uuid, payload_bytes, label = item
            self._update_queue_label()

            if not (self.client and self.client.is_connected):
                self._log(f"[Queue] Skipped '{label}': not connected", "error")
                self.send_queue.task_done()
                continue

            future = asyncio.run_coroutine_threadsafe(
                self._write_char(char_uuid, payload_bytes), self.ble_loop
            )
            try:
                future.result(timeout=10)
                self._log(f"[Queue] '{label}' sent ({len(payload_bytes)}B)", "success")
            except Exception as e:
                self._log(f"[Queue] '{label}' failed: {e}", "error")
            finally:
                self.send_queue.task_done()
                self._update_queue_label()

    async def _write_char(self, char_uuid: str, data: bytes):
        # Khắc phục lỗi "Multiple Characteristics" do Windows lưu Cache cũ
        target_char = None
        for service in self.client.services:
            for char in service.characteristics:
                if char.uuid.lower() == char_uuid.lower():
                    target_char = char
                    break
            if target_char:
                break
                
        if target_char is None:
            raise Exception(f"Characteristic {char_uuid} not found!")
            
        await self.client.write_gatt_char(target_char, data, response=True)

    # ── Enqueue ──────────────────────────────────────────────────────────────
    def enqueue_credentials(self):
        ssid = self.ssid_var.get().strip()
        pw   = self.pw_var.get().strip()

        if not ssid:
            self._log("Error: SSID cannot be empty.", "error")
            return
        if len(pw) < 8:
            self._log(f"Error: Password too short ({len(pw)}/min 8).", "error")
            return

        self.send_queue.put((CHAR_SSID_UUID, ssid.encode("utf-8"), f"SSID={ssid}"))
        self.send_queue.put((CHAR_PW_UUID,   pw.encode("utf-8"),   "PASSWORD"))

        self._log(f"[Queue] +2 packets — SSID:\"{ssid}\"  PW:{pw}", "queue")
        self._update_queue_label()

    def trigger_wifi_connection(self):
        self.send_queue.put((CHAR_CMD_UUID, b"\x01", "CMD_CONNECT"))
        self._log("[Queue] +1 packet — Command: Trigger Wi-Fi Connection", "queue")
        self._update_queue_label()

    def send_sensor_config(self):
        # Lấy giá trị từ các Spinbox
        y_str = self.cfg_year_var.get()
        mo_str = self.cfg_month_var.get()
        d_str = self.cfg_date_var.get()
        h_str = self.cfg_hour_var.get()
        m_str = self.cfg_min_var.get()
        s_str = self.cfg_sec_var.get()
        
        try:
            Y, Mo, D = int(y_str), int(mo_str), int(d_str)
            h, m, s = int(h_str), int(m_str), int(s_str)
            if not (2000 <= Y <= 2099 and 1 <= Mo <= 12 and 1 <= D <= 31):
                raise ValueError
            if not (0 <= h <= 23 and 0 <= m <= 59 and 0 <= s <= 59):
                raise ValueError
        except ValueError:
            self._log("Invalid Date/Time format. Please check the Spinbox values.", "error")
            return

        # Đóng gói thành chuỗi JSON
        # Ví dụ: {"Y": 2024, "Mo": 1, "D": 1, "H": 15, "M": 30, "S": 0}
        payload_dict = {"Y": Y, "Mo": Mo, "D": D, "H": h, "M": m, "S": s}
        payload_str = json.dumps(payload_dict)
        payload_bytes = payload_str.encode("utf-8")

        # Đẩy vào hàng đợi BLE để gửi xuống ESP32 thông qua SENSOR_CONFIG_UUID
        self.send_queue.put((CHAR_SENSOR_CONFIG_UUID, payload_bytes, f"CONFIG_RTC {Y:04}-{Mo:02}-{D:02} {h:02}:{m:02}:{s:02}"))
        self._log(f"[Queue] +1 packet — Sensor Config (Set to: {Y:04}-{Mo:02}-{D:02} {h:02}:{m:02}:{s:02})", "queue")
        self._update_queue_label()

    # ── Scan ─────────────────────────────────────────────────────────────────
    def scan_device(self):
        self._log("Scanning (4s)...", "info")
        self._run_async(self._scan_async())

    async def _scan_async(self):
        try:
            devices = await BleakScanner.discover(timeout=4.0)
            self.scanned_devices = {
                (d.name.strip() if d.name and d.name.strip() else d.address): d
                for d in devices
            }
            keys = list(self.scanned_devices.keys())
            self.root.after(0, lambda: self.device_selector.config(values=keys))
            if keys:
                self.root.after(0, lambda: self.device_selector.current(0))
                self._log(f"Found {len(keys)} device(s).", "success")
            else:
                self._log("No devices found.", "error")
        except Exception as e:
            self._log(f"Scan error: {e}", "error")

    # ── Connect ───────────────────────────────────────────────────────────────
    def connect_device(self):
        if self.is_connecting or self.connected:
            return
        selection = self.device_selector.get()
        if selection not in self.scanned_devices:
            self._log("Please scan and select a device first.", "error")
            return
        self.is_connecting = True
        self.manual_disconnect = False
        
        self.toggle_conn_btn.config(state="disabled")
        
        self.status_var.set("CONNECTING...")
        self.status_label.config(foreground="orange")
        self._run_async(self._connect_async(self.scanned_devices[selection]))

    async def _connect_async(self, device):
        try:
            self._log(f"Connecting to {device.address}...", "info")
            # Khai báo callback ngay lúc khởi tạo để tương thích với mọi phiên bản Bleak
            self.client = BleakClient(device, disconnected_callback=self._on_disconnect)
            await asyncio.wait_for(self.client.connect(), timeout=7.0)
            
            self.connected = True
            self._log(f"Connected to {device.name or device.address}", "success")
            
            # Chủ động báo cho giao diện (GUI) biết đã kết nối
            self.root.after(0, self._ui_set_connected)
            
            # Tìm Characteristic của Sensor và đăng ký nhận Notify
            target_char = None
            for service in self.client.services:
                for char in service.characteristics:
                    if char.uuid.lower() == CHAR_SENSOR_DATA_UUID.lower():
                        target_char = char
                        break
                if target_char:
                    break
            
            if target_char:
                await self.client.start_notify(target_char, self._notification_handler)
                self._log("Subscribed to Sensor Data notifications.", "success")
            else:
                self._log("Warning: Sensor Characteristic not found.", "warning")
                
        except asyncio.TimeoutError:
            self._log("Connection timed out.", "error")
            self.root.after(0, self._ui_set_disconnected)
        except Exception as e:
            self._log(f"Connection failed: {e}", "error")
            self.root.after(0, self._ui_set_disconnected)
        finally:
            self.is_connecting = False
            
    # ── Xử lý khi nhận được gói tin Notify từ ESP32 ─────────────────────────
    def _notification_handler(self, sender, data: bytes):
        try:
            # 1. Chuyển mảng byte thành chuỗi text
            payload_str = data.decode("utf-8")
            
            # 2. Giải mã JSON
            payload = json.loads(payload_str)
            
            # Chuyển đổi chuỗi "YYYY-MM-DD HH:MM:SS" thành định dạng "h:mm AM/PM" và "MM/DD/YYYY"
            time_str = payload.get("Time", "--")
            date_val, time_val = "--", "--"
            if time_str != "--":
                try:
                    dt = datetime.strptime(time_str, "%Y-%m-%d %H:%M:%S")
                    time_val = dt.strftime("%S:%M:%I %p") 
                    date_val = dt.strftime("%m/%d/%Y") 
                except ValueError:
                    if " " in time_str:
                        date_val, time_val = time_str.split(" ", 1)
                    else:
                        time_val = time_str
            
            # 3. Cập nhật các biến giao diện (đẩy vào luồng Tkinter)
            self.root.after(0, lambda: [
                self.sensor_date_var.set(date_val),
                self.sensor_time_var.set(time_val),
                self.sensor_temp_var.set(f"{payload.get('T', '--')} °C"),
                self.sensor_wifi_var.set("Connected" if payload.get("Wifi") else "Disconnected"),
                self.sensor_net_var.set("Available" if payload.get("Net") else "Offline"),
                self._reset_watchdog()
            ])
            
        except Exception as e:
            print(f"Error parsing sensor data: {e}")

    def _reset_watchdog(self):
        if self.watchdog_timer:
            self.root.after_cancel(self.watchdog_timer)
        if self.connected:
            self.watchdog_timer = self.root.after(5000, self._watchdog_timeout)

    def _watchdog_timeout(self):
        if self.connected:
            self._log("Connection timeout (no data from ESP32).", "error")
            self.disconnect_device()

    def _ui_set_connected(self):
        self.status_var.set("CONNECTED")
        self.status_label.config(foreground="green")
        self.device_selector.config(state="disabled")
        self.send_wifi_btn.config(state="normal")
        self.trigger_conn_btn.config(state="normal")
        self.btn_set_sensor.config(state="normal")
        self.toggle_conn_btn.config(text="Disconnect", command=self.disconnect_device, state="normal")
        self._reset_watchdog()

    def _ui_set_disconnected(self):
        self.status_var.set("DISCONNECTED")
        self.status_label.config(foreground="red")
        self.sensor_wifi_var.set("--")
        self.sensor_net_var.set("--")
        self.device_selector.config(state="readonly")
        self.send_wifi_btn.config(state="disabled")
        self.trigger_conn_btn.config(state="disabled")
        self.btn_set_sensor.config(state="disabled")
        self.toggle_conn_btn.config(text="Connect", command=self.connect_device, state="normal")
        if self.watchdog_timer:
            self.root.after_cancel(self.watchdog_timer)
            self.watchdog_timer = None

    def _on_disconnect(self, client):
        self.connected = False
        self._log("Device disconnected", "error" if not self.manual_disconnect else "info")
        self.root.after(0, self._ui_set_disconnected)

    # ── Disconnect ────────────────────────────────────────────────────────────
    def disconnect_device(self):
        if not self.client:
            return
        self.manual_disconnect = True
        self.toggle_conn_btn.config(state="disabled") 
        self._run_async(self._disconnect_async())

    async def _disconnect_async(self):
        try:
            if self.client:
                try:
                    await self._write_char(CHAR_CMD_UUID, b"\x02") # Gửi lệnh đá kết nối
                except:
                    pass
                await self.client.disconnect()
        finally:
            self.client = None

    # ── Cleanup ───────────────────────────────────────────────────────────────
    def on_close(self):
        # Ẩn giao diện ngay lập tức để người dùng có cảm giác app đã tắt tức thì
        self.root.withdraw()
        
        # Chủ động gửi lệnh ngắt kết nối BLE xuống hệ điều hành trước khi thoát
        if self.client and self.connected:
            try:
                async def graceful_disconnect():
                    if self.client.is_connected:
                        try:
                            await self._write_char(CHAR_CMD_UUID, b"\x02")
                        except:
                            pass
                        await self.client.disconnect()
                future = asyncio.run_coroutine_threadsafe(graceful_disconnect(), self.ble_loop)
                future.result(timeout=2.0) 
            except Exception as e:
                print(f"Error disconnecting on exit: {e}")
                
        self.send_queue.put(_STOP_SENTINEL)
        self.ble_loop.call_soon_threadsafe(self.ble_loop.stop)
        self.root.destroy()


if __name__ == "__main__":
    root = tk.Tk()
    app = BLEGuiApp(root)
    root.protocol("WM_DELETE_WINDOW", app.on_close)
    root.mainloop()