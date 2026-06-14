import tkinter as tk
import asyncio
import threading
from bleak import BleakScanner, BleakClient

class BLEConfigApp:
    def __init__(self, root):
        self.root = root
        self.root.title("ESP32 WiFi Config")
        self.root.geometry("400x500")

        self.client = None
        self.loop = asyncio.new_event_loop()
        
        # Khởi tạo UI
        self.setup_ui()
        
        # Chạy một thread riêng để quản lý các tác vụ Asyncio
        self.thread = threading.Thread(target=self.start_async_loop, daemon=True)
        self.thread.start()

    def setup_ui(self):
        tk.Label(self.root, text="Wifi SSID").pack()
        self.ssid_entry = tk.Entry(self.root)
        self.ssid_entry.pack()

        tk.Label(self.root, text="Password").pack()
        self.pass_entry = tk.Entry(self.root, show="*")
        self.pass_entry.pack()

        self.scan_btn = tk.Button(self.root, text="Scan BLE", command=self.on_scan)
        self.scan_btn.pack(pady=5)

        # =================== Connect Frame ===================
        button_frame = tk.Frame(self.root)
        button_frame.pack(pady=5)

        self.connect_btn = tk.Button(button_frame, text="Connect", command=self.on_connect)
        self.connect_btn.pack(side=tk.LEFT, padx=5)

        self.disconnect_btn = tk.Button(button_frame, text="Disconnect", command=self.on_disconnect)
        self.disconnect_btn.pack(side=tk.LEFT, padx=5)
        # =================== Connect Frame ===================

        self.send_btn = tk.Button(self.root, text="Send Wifi", command=self.on_send)
        self.send_btn.pack(pady=5)

        self.ota_btn = tk.Button(self.root, text="Update OTA", command=self.ota_update)
        self.ota_btn.pack(pady=5)

        self.device_list = tk.Listbox(self.root)
        self.device_list.pack(fill="both", expand=True)

        self.status_label = tk.Label(self.root, text="Status: Idle", fg="blue")
        self.status_label.pack(pady=10)

    # --- Quản lý Asyncio Loop ---
    def start_async_loop(self):
        asyncio.set_event_loop(self.loop)
        self.loop.run_forever()

    def run_async(self, coro):
        """Tiện ích để gửi task vào loop đang chạy"""
        asyncio.run_coroutine_threadsafe(coro, self.loop)

    # --- Các hàm nghiệp vụ ---
    async def scan_ble_logic(self):
        self.update_status("Scanning...")
        devices = await BleakScanner.discover()
        self.device_list.delete(0, tk.END)
        for d in devices:
            name = d.name if d.name else "Unknown"
            self.device_list.insert(tk.END, f"{name} - {d.address}")
        self.update_status("Scan complete")

    async def connect_logic(self, address):
        try:
            self.update_status(f"Connecting to {address}...")
            self.client = BleakClient(address)
            await self.client.connect()
            self.update_status("Connected!")
            
            # Đăng ký thông báo ngay sau khi kết nối
            # NOTIFY_UUID = "00009abc-0000-1000-8000-00805f9b34fb"
            # await self.client.start_notify(NOTIFY_UUID, self.notification_handler)
        except Exception as e:
            self.update_status(f"Error: {e}")

    async def disconnect_logic(self):
        try:
            if self.client and self.client.is_connected:

                await self.client.disconnect()
                self.update_status("Disconnected!")
            else:
                self.update_status("Not connected")
        except Exception as e:
            self.update_status(f"Disconnect error: {e}")

    async def send_wifi_logic(self):
        if not self.client or not self.client.is_connected:
            self.update_status("Error: Not connected!")
            return
        
        # SSID UUID
        WRITE_UUID = "6fc254b7-6bc0-475b-b836-720d2287922d"
        data = f"{self.ssid_entry.get()}".encode()
        await self.client.write_gatt_char(WRITE_UUID, data)
        # PASSWORD UUID
        WRITE_UUID = "cfedbbc2-0598-4581-b1fd-b7d54fec0396"
        data = f"{self.pass_entry.get()}".encode()
        await self.client.write_gatt_char(WRITE_UUID, data)
        self.update_status("Data sent!")

    def notification_handler(self, data):
        msg = data.decode()
        self.update_status(f"ESP32: {msg}")

    # --- Bridge giữa UI và Async ---
    def on_scan(self):
        self.run_async(self.scan_ble_logic())

    def on_connect(self):
        selected = self.device_list.get(tk.ACTIVE)
        if " - " in selected:
            address = selected.split(" - ")[1]
            self.run_async(self.connect_logic(address))

    def on_disconnect(self):
        self.run_async(self.disconnect_logic())

    def on_send(self):
        self.run_async(self.send_wifi_logic())
   
    def ota_update(self):
        pass

    def update_status(self, text):
        self.root.after(0, lambda: self.status_label.config(text=f"Status: {text}"))

if __name__ == "__main__":
    root = tk.Tk()
    app = BLEConfigApp(root)
    root.mainloop()