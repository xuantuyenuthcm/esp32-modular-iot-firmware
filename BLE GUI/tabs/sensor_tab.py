import tkinter as tk
import functools
from bleak import BleakScanner, BleakClient

# =================== Sensor tab ===================
class SensorControlTab(tk.Frame):
    def __init__(self, parent, app):
        super().__init__(parent)
        self.app = app

        self.sensor_commands = {
            "AHT20":  0,
            "BH1750": 1,
            "BMP280": 2,
            "BNO055": 3,
            "INA226": 4
        }
        self.canvases = {}
        self.lights = {}
        for name, cmd in self.sensor_commands.items():
            # Frame
            frame_status = tk.Frame(self)
            frame_status.pack(fill="x", padx=5, pady=2)
            # Light status
            status_canvas = tk.Canvas(frame_status, width=20, height=20, highlightthickness=0)
            status_canvas.grid(row=0, column=0, padx=(0, 5), pady=2)
            light = status_canvas.create_oval(2, 2, 18, 18, fill="red", outline="darkred")
            # Label
            label = tk.Label(frame_status, text=name, anchor="w", width=7)
            label.grid(row=0, column=1, sticky="w", pady=2)
            # Button
            connect_sensor_btn = tk.Button(frame_status, text="Connect", command=functools.partial(self.on_send_command, name, cmd))
            connect_sensor_btn.grid(row=0, column=2, padx=5, pady=2)
            # Save to array
            self.canvases[name] = status_canvas
            self.lights[name] = light
        # Connect all button
        frame_connect_all = tk.Frame(self)
        frame_connect_all.pack(fill="x", padx=5, pady=5)
        connect_all_sensor_btn = tk.Button(frame_connect_all, text="Connect All", width=19)
        connect_all_sensor_btn.pack(side="left")
        # Disconnect all button
        frame_disconnect_all = tk.Frame(self)
        frame_disconnect_all.pack(fill="x", padx=5, pady=5)
        disconnect_all_sensor_btn = tk.Button(frame_disconnect_all, text="Disconnect All", width=19)
        disconnect_all_sensor_btn.pack(side="left")
        # code below is for connect
        # self.status_canvas.itemconfig(self.status_light, fill="green", outline="darkgreen")
        # self.status_canvas.itemconfig(self.status_light, fill="red", outline="darkred")

    def on_send_command(self, sensor_name, command_int):
        if not self.app.client or not self.app.client.is_connected:
            self.app.update_status("Error: Not connected!")
            return
        
        self.app.run_async(self.send_command_logic(sensor_name, command_int))

    async def send_command_logic(self, sensor_name, command_int):
        try:
            # convert int to a byte
            WRITE_UUID = "45fb1ea9-6f80-48a4-9337-4e8e4d455e2c"
            data = bytes([command_int])
            await self.app.client.write_gatt_char(WRITE_UUID, data)
            self.app.update_status("Command sent!")
        except Exception as e:
            self.app.update_status(f"Error: {e}")

    def update_sensor_status(self, sensor_id: int, status: int):
        sensor_name = None
        for name, cmd in self.sensor_commands.items():
            if cmd == sensor_id:
                sensor_name = name
                break  
        
        if sensor_name and sensor_name in self.lights:
            canvas = self.canvases[sensor_name]
            light  = self.lights[sensor_name]

            if status == 0x00:
                self.app.root.after(0, lambda: canvas.itemconfig(light, fill="lightgreen", outline="darkgreen"))
                self.app.update_status(f"Sensor {sensor_name} connected!")
            else:
                self.app.root.after(0, lambda: canvas.itemconfig(light, fill="red", outline="darkred"))
                self.app.update_status(f"Sensor {sensor_name} init failed!")
        # =================== Sensor tab ===================