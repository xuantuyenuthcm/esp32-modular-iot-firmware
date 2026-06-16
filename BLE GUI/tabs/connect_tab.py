import tkinter as tk
from bleak import BleakScanner, BleakClient

class ConnectTab(tk.Frame):
    def __init__(self, parent, app):
        super().__init__(parent)
        self.app = app

        # =================== Connect Bluetooth Frame ===================
        self.button_frame_connect_btn = tk.Frame(self)
        self.button_frame_connect_btn.pack(padx=10, pady=5, anchor="w")

        self.scan_btn = tk.Button(self.button_frame_connect_btn, text="Scan", command=self.on_scan)
        self.scan_btn.pack(side="left", padx=(0, 5), ipadx=10, ipady=3)

        self.connect_btn = tk.Button(self.button_frame_connect_btn, text="Connect", command=self.on_connect)
        self.connect_btn.pack(side="left", padx=(0, 5), ipadx=10, ipady=3)

        self.disconnect_btn = tk.Button(self.button_frame_connect_btn, text="Disconnect", command=self.on_disconnect)
        self.disconnect_btn.pack(side="left", padx=(0, 5), ipadx=10, ipady=3)

        self.device_list = tk.Listbox(self)
        self.device_list.pack(fill="both", expand=True, padx=10)
        # =================== Connect Bluetooth Frame =================== 

    def notification_handler(self, sender, data):
        msg = data.decode()
        self.app.update_status(f"ESP32: {msg}")

        if len(data) >= 2:
            sensor_id = data[0]
            status = data[1]
            self.app.tab_sensor_control.update_sensor_status(sensor_id, status)


    # --- Bridge giữa UI và Async ---
    def on_scan(self):
        self.app.run_async(self.scan_ble_logic())

    def on_connect(self):
        selected = self.device_list.get(tk.ACTIVE)
        if " - " in selected:
            address = selected.split(" - ")[1]
            self.app.run_async(self.connect_logic(address))

    def on_disconnect(self):
        self.app.run_async(self.disconnect_logic())

    def on_send(self):
        self.app.run_async(self.send_wifi_logic())

    def ota_update(self):
        pass

    async def scan_ble_logic(self):
        self.app.update_status("Scanning...")
        devices = await BleakScanner.discover()
        self.device_list.delete(0, tk.END)
        for d in devices:
            name = d.name if d.name else "Unknown"
            self.device_list.insert(tk.END, f"{name} - {d.address}")
        self.app.update_status("Scan complete")

    async def connect_logic(self, address):
        try:
            self.app.update_status(f"Connecting to {address}...")
            self.app.client = BleakClient(address)
            await self.app.client.connect()
            self.app.update_status("Connected!")
            
            # Register notify
            NOTIFY_UUID = "45fb1ea9-6f80-48a4-9337-4e8e4d455e2c"
            await self.app.client.start_notify(NOTIFY_UUID, self.notification_handler)
        except Exception as e:
            self.app.update_status(f"Error: {e}")

    async def disconnect_logic(self):
        try:
            if self.app.client and self.app.client.is_connected:
                await self.app.client.disconnect()
                self.app.update_status("Disconnected!")
            else:
                self.app.update_status("Not connected")
        except Exception as e:
            self.app.update_status(f"Disconnect error: {e}")

    async def send_wifi_logic(self):
        if not self.app.client or not self.app.client.is_connected:
            self.app.update_status("Error: Not connected!")
            return
        
        # SSID UUID
        WRITE_UUID = "6fc254b7-6bc0-475b-b836-720d2287922d"
        data = f"{self.ssid_entry.get()}".encode()
        await self.app.client.write_gatt_char(WRITE_UUID, data)
        # PASSWORD UUID
        WRITE_UUID = "cfedbbc2-0598-4581-b1fd-b7d54fec0396"
        data = f"{self.pass_entry.get()}".encode()
        await self.app.client.write_gatt_char(WRITE_UUID, data)
        self.app.update_status("Data sent!")