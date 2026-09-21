#!/usr/bin/env python3
"""FinOS Setup — install qFin (desktop) and flash T-Embed firmware."""
import os, sys, subprocess, threading, webbrowser

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))

try:
    import tkinter as tk
    from tkinter import ttk, filedialog, messagebox
except ImportError:
    print("tkinter is required")
    sys.exit(1)


class App(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("FinOS Setup")
        self.geometry("520x420")
        self.configure(bg="#0c0c0d")
        fg, ac = "#f4f1ea", "#ff8200"
        tk.Label(self, text="FinOS Setup", fg=ac, bg="#0c0c0d",
                 font=("Courier", 22)).pack(pady=12)
        tk.Label(self, text="Firmware + qFin desktop for T-Embed CC1101 / Plus",
                 fg=fg, bg="#0c0c0d").pack()
        self.log = tk.Text(self, height=12, bg="#070708", fg="#b6ffb0",
                           insertbackground=fg, font=("Courier", 10))
        self.log.pack(fill="both", expand=True, padx=16, pady=8)
        row = tk.Frame(self, bg="#0c0c0d")
        row.pack(pady=8)
        tk.Button(row, text="Install qFin (npm)", command=self.install_qfin,
                  bg=ac, fg="#1a0e00").pack(side="left", padx=4)
        tk.Button(row, text="Flash firmware…", command=self.flash,
                  bg="#1c1c20", fg=fg).pack(side="left", padx=4)
        tk.Button(row, text="Open web installer", command=lambda:
                  webbrowser.open("https://fak3hidden.github.io/Firmware-test/install.html"),
                  bg="#1c1c20", fg=fg).pack(side="left", padx=4)

    def say(self, s):
        self.log.insert("end", s + "\n")
        self.log.see("end")

    def install_qfin(self):
        def run():
            desk = os.path.join(ROOT, "desktop")
            self.say("npm install in " + desk)
            r = subprocess.run(["npm", "install"], cwd=desk, capture_output=True, text=True)
            self.say(r.stdout[-500:] if r.stdout else "")
            self.say("exit %s" % r.returncode)
            if r.returncode == 0:
                self.say("Starting qFin…")
                subprocess.Popen(["npm", "start"], cwd=desk)
        threading.Thread(target=run, daemon=True).start()

    def flash(self):
        path = filedialog.askopenfilename(title="Merged FinOS .bin",
                                          filetypes=[("bin", "*.bin")])
        if not path:
            return
        port = filedialog.askstring("Port", "Serial port (e.g. COM3 or /dev/ttyACM0)")
        if not port:
            return
        def run():
            self.say("esptool write_flash 0x0 " + path)
            r = subprocess.run(
                [sys.executable, "-m", "esptool", "--chip", "esp32s3", "-p", port,
                 "-b", "921600", "write_flash", "-z", "--flash_mode", "qio",
                 "--flash_size", "16MB", "0x0", path],
                capture_output=True, text=True)
            self.say(r.stdout[-800:] if r.stdout else r.stderr[-800:])
            self.say("exit %s" % r.returncode)
        threading.Thread(target=run, daemon=True).start()


if __name__ == "__main__":
    App().mainloop()
