#include <raylib.h>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <thread>
#include <atomic>
#include <cstring>
#include <fstream>
#include <unistd.h>
#include <random>
#include "libs/discord_rpc.h"

namespace fs = std::filesystem;

// --- GESTIÓN DE RUTAS E ICONOS (INTACTO) ---
struct PathManager {
    static std::string GetHome() {
        const char* home = getenv("HOME");
        return home ? std::string(home) : ".";
    }

    static void SetupDesktopEntry(const char* exePath) {
        fs::path pExe(exePath);
        std::string home = GetHome();
        
        std::string iconDir = home + "/.local/share/applications/icon";
        std::string iconPath = iconDir + "/icon.png";
        
        try {
            fs::create_directories(iconDir);
            std::string srcIcon = pExe.parent_path().string() + "/icon/icon.png";
            
            if (fs::exists(srcIcon)) {
                fs::copy_file(srcIcon, iconPath, fs::copy_options::overwrite_existing);
                fs::permissions(iconPath, fs::perms::owner_read | fs::perms::group_read | fs::perms::others_read);
            }
        } catch (...) {}

        std::string desktopPath = home + "/.local/share/applications/kdlplayer.desktop";
        std::ofstream df(desktopPath, std::ios::trunc); 
        if (df.is_open()) {
            df << "[Desktop Entry]\n"
               << "Type=Application\n"
               << "Name=kdlplayer\n"
               << "Comment=Music Player by Ars Byte\n"
               << "Exec=" << exePath << "\n"
               << "Icon=" << iconPath << "\n" 
               << "Path=" << pExe.parent_path().string() << "\n"
               << "Terminal=false\n"
               << "Categories=AudioVideo;Player;\n"
               << "StartupNotify=true\n";
            df.close();
            fs::permissions(desktopPath, fs::perms::owner_read | fs::perms::owner_write | fs::perms::owner_exec);
        }
        system("update-desktop-database ~/.local/share/applications/ > /dev/null 2>&1");
    }
};

// --- DIÁLOGO NATIVO (INTACTO) ---
std::string PickFolderDialog() {
    char path[1024];
    FILE *f = popen("zenity --file-selection --directory --title='kdlplayer - Seleccionar Música'", "r");
    if (!f) return "";
    if (fgets(path, 1024, f)) {
        std::string s(path);
        s.erase(std::remove(s.begin(), s.end(), '\n'), s.end());
        pclose(f);
        return s;
    }
    pclose(f);
    return "";
}

// --- DISCORD RICH PRESENCE (INTACTO) ---
struct DiscordManager {
    const char* APPLICATION_ID = "1468317452969709570";

    void Init() {
        DiscordEventHandlers handlers;
        memset(&handlers, 0, sizeof(handlers));
        Discord_Initialize(APPLICATION_ID, &handlers, 1, NULL);
    }

    void Update(const std::string& songName, bool isPaused, float currentPos, float totalLen) {
        DiscordRichPresence discordPresence;
        memset(&discordPresence, 0, sizeof(discordPresence));
        discordPresence.details = songName.c_str();
        discordPresence.state = isPaused ? "En Pausa" : "by: RomanticHomicide";
        discordPresence.largeImageKey = "logo"; 
        
        if (!isPaused && totalLen > 0) {
            time_t now = time(NULL);
            discordPresence.startTimestamp = now - (long)currentPos;
            discordPresence.endTimestamp = now + (long)(totalLen - currentPos);
        }
        Discord_UpdatePresence(&discordPresence);
    }

    void Shutdown() { Discord_Shutdown(); }
};

// --- LÓGICA DEL REPRODUCTOR (OPTIMIZADA) ---
struct Song { std::string path, name; };

class Reproductor {
private:
    std::mt19937 rng{std::random_device{}()};
public:
    std::vector<Song> playlist;
    int currentIndex = 0;
    Music currentMusic = { 0 };
    std::atomic<bool> isLoaded{false}, isPaused{false}, quit{false};
    bool isLooping = false, isShuffle = false;
    float volume = 0.5f;

    void loadPlaylist(const std::string& folder) {
        if (!fs::exists(folder)) return;
        if (isLoaded) { UnloadMusicStream(currentMusic); isLoaded = false; }
        playlist.clear();

        for (const auto& entry : fs::directory_iterator(folder)) {
            auto ext = entry.path().extension().string();
            if (ext == ".mp3" || ext == ".flac" || ext == ".ogg") 
                playlist.push_back({entry.path().string(), entry.path().filename().string()});
        }
        
        if (playlist.empty()) return;
        std::sort(playlist.begin(), playlist.end(), [](const auto& a, const auto& b) { return a.name < b.name; });
        currentIndex = 0;
        play();
    }

    void play() {
        if (isLoaded) UnloadMusicStream(currentMusic);
        currentMusic = LoadMusicStream(playlist[currentIndex].path.c_str());
        if (currentMusic.stream.buffer != nullptr) { 
            PlayMusicStream(currentMusic);
            SetMusicVolume(currentMusic, volume);
            isLoaded = true; isPaused = false;
        }
    }

    void siguiente() {
        if (playlist.empty()) return;
        if (isShuffle && playlist.size() > 1) {
            int nextIdx;
            do { nextIdx = rng() % playlist.size(); } while (nextIdx == currentIndex);
            currentIndex = nextIdx;
        } else currentIndex = (currentIndex + 1) % playlist.size();
        play();
    }

    void anterior() {
        if (playlist.empty()) return;
        currentIndex = (currentIndex - 1 + (int)playlist.size()) % playlist.size();
        play();
    }
    
    void togglePause() {
        if (!isLoaded) return;
        isPaused = !isPaused;
        if (isPaused) PauseMusicStream(currentMusic); else ResumeMusicStream(currentMusic);
    }
};

void AudioThread(Reproductor* p) {
    while (!p->quit) {
        if (p->isLoaded && !p->isPaused) UpdateMusicStream(p->currentMusic);
        std::this_thread::sleep_for(std::chrono::milliseconds(8));
    }
}

int main(int argc, char* argv[]) {
    char fullPath[1024];
    if (realpath(argv[0], fullPath)) PathManager::SetupDesktopEntry(fullPath);

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(550, 280, "kdlplayer - MAIKRA EDITION");
    InitAudioDevice();

    // --- CARGAR FUENTE PERSONALIZADA ---
    // Asegúrate de tener un archivo 'font.ttf' en la carpeta. Si no lo encuentra, usa la de por defecto.
    Font customFont = LoadFontEx("font.ttf", 24, 0, 0); 
    
    DiscordManager discord;
    discord.Init();

    Reproductor player;
    player.loadPlaylist(PathManager::GetHome() + "/Music");

    std::thread audioUpdater(AudioThread, &player);
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        // INTERACTIVIDAD: Rueda del ratón para volumen
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) {
            player.volume = std::clamp(player.volume + (wheel * 0.05f), 0.0f, 1.0f);
            if (player.isLoaded) SetMusicVolume(player.currentMusic, player.volume);
        }

        // Atajos de teclado originales
        if (IsKeyDown(KEY_UP)) SetMusicVolume(player.currentMusic, player.volume = std::clamp(player.volume + 0.01f, 0.0f, 1.0f));
        if (IsKeyDown(KEY_DOWN)) SetMusicVolume(player.currentMusic, player.volume = std::clamp(player.volume - 0.01f, 0.0f, 1.0f));
        if (IsKeyPressed(KEY_RIGHT)) player.siguiente();
        if (IsKeyPressed(KEY_LEFT)) player.anterior();
        if (IsKeyPressed(KEY_SPACE)) player.togglePause();
        if (IsKeyPressed(KEY_L)) player.isLooping = !player.isLooping;
        if (IsKeyPressed(KEY_R)) player.isShuffle = !player.isShuffle;
        if (IsKeyPressed(KEY_O)) {
            std::string folder = PickFolderDialog();
            if (!folder.empty()) player.loadPlaylist(folder);
        }

        // INTERACTIVIDAD: Clic en la barra de progreso
        Rectangle progressBarHitbox = {25, 160, 500, 24}; // Área más grande para facilitar el clic
        Vector2 mousePos = GetMousePosition();
        
        if (player.isLoaded && CheckCollisionPointRec(mousePos, progressBarHitbox)) {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                float clickX = mousePos.x - 25.0f;
                float progressTarget = clickX / 500.0f;
                float timeTarget = progressTarget * GetMusicTimeLength(player.currentMusic);
                SeekMusicStream(player.currentMusic, timeTarget);
            }
        } else if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && mousePos.y > 50) {
            // Click fuera de la barra y del header para pausar/reproducir
            player.togglePause();
        }

        // Lógica de reproducción y Discord
        if (player.isLoaded) {
            if (!player.isPaused) {
                if (GetMusicTimePlayed(player.currentMusic) >= GetMusicTimeLength(player.currentMusic) - 0.15f) player.siguiente();
                discord.Update(player.playlist[player.currentIndex].name, false, GetMusicTimePlayed(player.currentMusic), GetMusicTimeLength(player.currentMusic));
            } else {
                discord.Update(player.playlist[player.currentIndex].name, true, 0, 0);
            }
        }
        Discord_RunCallbacks();

        // --- RENDERIZADO ---
        BeginDrawing();
            ClearBackground({18, 18, 18, 255});
            DrawRectangle(0, 0, 550, 50, {35, 35, 35, 255});
            
            // Usando la nueva fuente para el Header
            DrawTextEx(customFont, "kdlplayer", {20, 15}, 22, 1, SKYBLUE);
            DrawTextEx(customFont, "By Ars Byte", {440, 20}, 12, 1, DARKGRAY);

            if (player.isLoaded) {
                // Título de la canción
                DrawTextEx(customFont, player.playlist[player.currentIndex].name.c_str(), {25, 80}, 18, 1, RAYWHITE);
                
                // Barra de progreso interactiva
                float length = GetMusicTimeLength(player.currentMusic);
                float prog = (length > 0) ? GetMusicTimePlayed(player.currentMusic) / length : 0;
                
                // Hover effect visual si el ratón está sobre la barra
                Color barColor = CheckCollisionPointRec(mousePos, progressBarHitbox) ? LIGHTGRAY : SKYBLUE;
                
                DrawRectangle(25, 170, 500, 4, {45, 45, 45, 255}); // Fondo barra
                DrawRectangle(25, 170, (int)(500 * prog), 4, barColor); // Progreso
                DrawCircle(25 + (int)(500 * prog), 172, 5, RAYWHITE); // Indicador
            }
            
            // Controles usando la nueva fuente
            DrawTextEx(customFont, "O: Abrir | R: Shuffle | L: Loop | SPACE/CLICK: Pausa | SCROLL: Vol", {30, 257}, 12, 1, DARKGRAY);
            
            // Indicadores de estado visuales
            if (player.isShuffle) DrawTextEx(customFont, "SHUFFLE ON", {450, 240}, 10, 1, SKYBLUE);
            if (player.isLooping) DrawTextEx(customFont, "LOOP ON", {450, 255}, 10, 1, SKYBLUE);

        EndDrawing();
    }

    player.quit = true;
    if (audioUpdater.joinable()) audioUpdater.join();
    discord.Shutdown();
    UnloadFont(customFont); // Limpieza de memoria de la fuente
    CloseAudioDevice();
    CloseWindow();
    return 0;
}