import tkinter as tk
from tkinter import ttk
import asyncio
import threading
from bleak import BleakScanner, BleakClient
from tabs.connect_tab import ConnectTab
from tabs.sensor_tab import SensorControlTab

class BLEConfigApp:
    def __init__(self, root):
        self.root = root
        self.root.title("ESP32 Sensors Monitor")
        self.root.geometry("800x400")

        self.client = None
        self.loop = asyncio.new_event_loop()
        
        # Khởi tạo UI
        self.setup_ui()
        
        # Chạy một thread riêng để quản lý các tác vụ Asyncio
        self.thread = threading.Thread(target=self.start_async_loop, daemon=True)
        self.thread.start()

    def setup_ui(self):
        self.status_label = tk.Label(self.root, text="Status: Idle", fg="blue", bd=1, relief=tk.SUNKEN, anchor="w")
        self.status_label.pack(side="bottom", fill="x")   
        
        # =================== Tab manager ===================
        self.notebook = ttk.Notebook(self.root)
        self.notebook.pack(fill="both", expand="True", padx=10, pady=0)

        self.tab_connect_bluetooth = ConnectTab(self.notebook, self)
        self.notebook.add(self.tab_connect_bluetooth, text="Connect")
        self.tab_sensor_control = SensorControlTab(self.notebook, self)
        self.notebook.add(self.tab_sensor_control, text="Sensors")
        # =================== Tab manager ===================    


        # tk.Label(self.root, text="Wifi SSID").pack(anchor= "w", padx=10)
        # self.ssid_entry = tk.Entry(self.root)
        # self.ssid_entry.pack(anchor="w", padx=10)

        # tk.Label(self.root, text="Password").pack(anchor= "w", padx=10)
        # self.pass_entry = tk.Entry(self.root, show="*")
        # self.pass_entry.pack(anchor="w", padx=10)

        # self.send_btn = tk.Button(self.root, text="Send Wifi", command=self.on_send)
        # self.send_btn.pack(pady=5)

        # self.ota_btn = tk.Button(self.root, text="Update OTA", command=self.ota_update)
        # self.ota_btn.pack(pady=5)
                # --- Quản lý Asyncio Loop ---
    def start_async_loop(self):
        asyncio.set_event_loop(self.loop)
        self.loop.run_forever()

    def run_async(self, coro):
        """Tiện ích để gửi task vào loop đang chạy"""
        asyncio.run_coroutine_threadsafe(coro, self.loop)

    def update_status(self, text):
        self.root.after(0, lambda: self.status_label.config(text=f"Status: {text}"))


if __name__ == "__main__":
    root = tk.Tk()
    app = BLEConfigApp(root)
    root.mainloop()