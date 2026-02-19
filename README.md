# kdlplayer 🎵

**kdlplayer** is a minimalist and efficient music player developed in C++ using the **raylib** library. Designed specifically for Linux systems, it offers a clean interface, keyboard controls, and automatic desktop integration.

---

## ✨ Features

* **Format Support:** Plays `.mp3` files.
* **Playlist Management:** Loads entire folders and sorts songs alphabetically.
* **Playback Modes:** Includes **Shuffle** and **Loop** functionality.
* **GUI:** Real-time progress visualization and intuitive click-to-seek controls.
* **Linux Integration:** For all Linux distributions
* Automatically generates a `.desktop` file to appear in your application launcher.
* Uses native system dialogues (**Zenity**) for folder selection.


* **Background Audio:** Dedicated audio thread processing to ensure smooth playback.

---

## 🛠️ Prerequisites

To compile and run this project, you will need:

1. **Raylib:** Installed on your system.
2. **Zenity:** For folder selection dialogs (`sudo apt install zenity`).
3. **C++ Compiler:** Supporting C++17 or higher.
4. **Assets:** The program expects `icon/font.ttf` and `icon/icon.png` for the UI.

---

## 🚀 Compilation

You can compile the project using `g++`. Ensure you are in the project's root directory:

```bash
g++ mixerplayer.cpp -o kdlplayer -L./libs -ldiscord-rpc -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -std=c++17

```

*Note: The code references `libs/discord_rpc.h`. Ensure you link the provided shared library (`libdiscord-rpc.so`) if you intend to enable Discord Rich Presence.*

---

## 🎮 Controls

| Action | Control |
| --- | --- |
| **Open Folder** | `O` Key |
| **Pause / Resume** | Left Click on screen |
| **Next / Previous** | Right (`→`) / Left (`←`) Arrows |
| **Toggle Shuffle** | `R` Key |
| **Toggle Loop** | `L` Key |
| **Adjust Volume** | Mouse Wheel |
| **Seek Position** | Click on the Progress Bar |

---

## 📂 Project Structure

```text
.
├── kdlplayer            # Generated executable
├── mixerplayer.cpp      # Main source code
├── icon/                # Asset folder
│   ├── font.ttf         # Custom font
│   └── icon.png         # Application icon
└── libs/                # External libraries
    ├── discord_rpc.h    # Discord RPC header
    └── libdiscord-rpc.so # Shared library

```

---

## 📝 Installation Notes

Upon the first execution, the program attempts to create a shortcut at:
`~/.local/share/applications/kdlplayer.desktop`

This allows you to search for "kdlplayer" directly from your Linux application menu.


## Preview:

<img width="565" height="291" alt="image" src="https://github.com/user-attachments/assets/6f401d23-873c-4687-8d11-41272e958895" />

---

**Developed by:** Ars Byte :b
