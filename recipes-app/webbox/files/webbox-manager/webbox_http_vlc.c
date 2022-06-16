#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/un.h>
#include <dirent.h> 
#include <sys/wait.h>
#include <vlc/vlc.h>
#include <vlc/libvlc_media_player.h>

#include "webbox_http.h"

#define LOG_NAME "VLC: "

#define CONFIG_PATH "/home/weston/.webbox"
#define PLAYLIST_SKIP CONFIG_PATH "/playlists.skip"
#define PLAYLIST_LATEST CONFIG_PATH "/playlists.latest"

#define PLAYLIST_PATH WEBBOX_LIB_PATH "/playlists"

// Only one VLC instance is running at time and looping a playlist
static int vlc_pid = -1;

typedef struct playlist {
    char *folder;
    struct playlist *more;
} playlist;

static void playlist_clear(playlist *p) {
    while (p) {
        playlist *next = p->more;
        free(p->folder);
        free(p);
        p = next;
    }
}

// use "" for empty folder/name
static char *get_path(const char* const folder, const char* const name) {
    char *filename = NULL;

    filename = malloc(strlen(PLAYLIST_PATH) + strlen(folder) + 1 + strlen(name) + 1); // +1 for '/' and '\0'
    if (filename) {
        (void)sprintf(filename, "%s%s%s", PLAYLIST_PATH, folder, name);
    }

    return filename;
}

static char *read_file(const char* const filename) {
    FILE *file = NULL;
    char *contents = NULL;

    file = fopen(filename, "rt");
    if (!file) {
        goto out;
    }
    
    if (fseek(file, 0, SEEK_END) != 0) {
        goto out;
    }
    long length = ftell(file);
    if (length == -1) {
        goto out;
    }
    if (fseek (file, 0, SEEK_SET) != 0) {
        goto out;
    }

    contents = malloc(length+1);
    if (!contents) {
        goto out;
    }
    if (fread(contents, 1, length, file) != length) {
        goto out;
    }
    contents[length] = '\0';
    printf(LOG_NAME "Read %s=%s\n", filename, contents);

out:
    if (!contents) {
        printf(LOG_NAME "No file %s\n", filename);
    }
    if (file) {
        fclose(file);
    }

    return contents;
}

static void file_write_line(FILE *file, const char* const line) {
    int length = strlen(line);
    if (fwrite(line, 1, length, file) != length) {
        printf(LOG_NAME "Write line failed %s\n", line);
    }
    if (fwrite("\n", 1, 1, file) != 1) {
        printf(LOG_NAME "Write line failed\n");
    }
}

static void write_file(const char* const filename, const char* const contents) {
    FILE *file = NULL;

    file = fopen(filename, "wt");
    if (file) {
        int length = strlen(contents);
        if (fwrite(contents, 1, length, file) == length) {
            printf(LOG_NAME "Write %s: %s\n", filename, contents);
        } else {
            printf(LOG_NAME "Write failed %s: %s\n", filename, contents);
        }
        fclose(file);
    }
}

static int dir_filter(const struct dirent *entry) {
   return (entry->d_type == DT_DIR && entry->d_name[0] != '.');
}

static int file_filter(const struct dirent *entry) {
    if (entry->d_type == DT_DIR) { // to recurse
        return 1;
    }
    if (entry->d_type != DT_REG) {
        return 0;
    }
    char *types[] =  { "wav", "aac", "mp3", "wma", "flac", "m4a" };
    for (int i=0; i<sizeof(types)/sizeof(types[0]); i++) {
        char *type = types[i];
        int len = strlen(entry->d_name);
        int n = strlen(type);
        if (len > n) {
            if (strcmp(entry->d_name + len - n, type) == 0) {
                return 1;
            }
        }
    }
    return 0;
}

playlist *get_dir(playlist *playlists, const char* const folder, bool recurse) {
    char *filename = NULL;

    filename = get_path(folder, "");
    if (!filename) {
        return NULL;
    }

    struct dirent **namelist;
    //printf("get_dir: %s\n", filename);
    int n = scandir(filename, &namelist, (recurse) ? file_filter : dir_filter, alphasort);
    if (n < 0) {
        perror("scandir");
        goto out;
    }

    playlist *last = playlists;
    while (last && last->more) {
        last = last->more;
    }

    for (int i=0; i<n; i++) {
        if (recurse && namelist[i]->d_type == DT_DIR) {
            if (namelist[i]->d_name[0] != '.') {
                char *path = malloc(strlen(folder) + 1 + strlen(namelist[i]->d_name) + 1);
                (void)sprintf(path, "%s/%s", folder, namelist[i]->d_name);
                playlists = get_dir(playlists, path, recurse);
                free(path);
            }
            continue;
        }

        playlist *p = (playlist *)malloc(sizeof(playlist));
        p->folder = malloc(strlen(filename) + 1 + strlen(namelist[i]->d_name) + 1);
        (void)sprintf(p->folder, "%s/%s", filename, namelist[i]->d_name);
        //printf("file %s\n", p->folder);
        free(namelist[i]);

        if (last) {
            last->more = p;
        } else {
            playlists = p;
            last = playlists;
        }
        last = p;
    }
    if (last) {
        last->more = NULL;
    }
    free(namelist);

out:
    free(filename);

    return playlists;
}

playlist *latest_get(playlist *list, const char* const pl) {
    if (!list) {
        return NULL;
    }
    
    char *latests = read_file(PLAYLIST_LATEST);
    if (!latests) {
        return list;
    }

    // Find latest for this playlist
    char *latest = strtok(latests, "\n");
    while(latest) {
        if (strncmp(latest, pl, strlen(pl)) == 0) {
            break;
        }
        latest = strtok(NULL, "\n");
    }

    if (!latest) {
        free(latests);
        return list;
    }

    playlist *p = list;
    while (p) {
        if (strcmp(p->folder, latest) == 0) {
            break;
        }
        p = p->more;
    }
    free(latests);

    if (!p) {
        return list;
    }

    //printf("latest %s\n", p->folder);
    return p;
}

// folder needed to remove existing entries
void latest_write(const char* const folder, const char* const filename) {
    printf("latest_write %s\n", filename);
    char *latests = read_file(PLAYLIST_LATEST);
    FILE *file = fopen(PLAYLIST_LATEST, "wt");
    if (file) {
        //char *fname = get_path("", filename); // filename has also folder
        file_write_line(file, filename);
        //free(fname);
        if (latests) {
            char *remove = get_path(folder, "");
            char *latest = strtok(latests, "\n");
            while(latest) {
                if (strncmp(remove, latest, strlen(remove)) != 0) {
                    file_write_line(file, latest);
                }
                latest = strtok(NULL, "\n");
            }
            free(remove);
            free(latests);
        }
        fclose(file);
    }
}

static int start_vlc(const char const *start_folder) {
    int pid = fork();
    char *folder = strdup(start_folder);
    switch (pid) {
        case -1:
            perror("VLC");
            break;
        case 0:
            // child process is run as "weston" for audio resources
            setuid(1000);
            setgid(1000);
            putenv("HOME=/home/weston");

            libvlc_instance_t *inst = libvlc_new (0, NULL);
            if (!inst) {
                exit(EXIT_FAILURE);
            }
            libvlc_media_player_t *mp;
            playlist *files = get_dir(NULL, folder, true);

            // in the future audio files may come from many drives so prefer full paths
            char *pf = get_path("", folder);
            playlist *p = latest_get(files, pf);
            free(pf);

            char *fskiplist = read_file(PLAYLIST_SKIP);

            if (p->more) { // Skip the latest (also skips the first one)
                p = p->more;
            } else {
                p = files;
            }

            while (p) { // loop playlist
                char *filename = p->folder;
                if (access(filename, F_OK ) != 0) {
                    printf("Check file read permission %s\n", filename);
                    p = p->more;
                    continue;
                }

                if (fskiplist) {
                    char *line = strtok(fskiplist, "\n");
                    bool skip = false;
                    while(line) {
                        if (strcmp(line, filename) == 0) {
                            skip = true;
                            break;
                        }
                        line = strtok(NULL, "\n");
                    }
                    if (skip) {
                        printf("Skip %s\n", filename);
                        p = p->more;
                        continue;
                    }
                }

                latest_write(folder, p->folder);

#ifndef VLC_EXEC
                printf(LOG_NAME "No audio play on host %s.\n", filename);
                //free(filename);
                playlist_clear(files);
                free(fskiplist);
                free(folder);
                exit(EXIT_SUCCESS);
#endif

                libvlc_media_t *m;
                //m = libvlc_media_new_location (inst, url);
                m = libvlc_media_new_path (inst, filename);
                free(filename);
                if (!m) {
                    printf(LOG_NAME "Media failure\n");
                    exit(EXIT_FAILURE);
                }

                mp = libvlc_media_player_new_from_media (m);
                if (mp) {
                   if (libvlc_media_player_play (mp) == -1) {
                        printf("VLC play failed!");
                        exit(EXIT_FAILURE);
                   }
                }
printf("time %d\n", (int)(ceil(libvlc_media_get_length(m)/1000)));

                libvlc_media_parse (m); // need to call before libvlc_media_get_duration
libvlc_state_t state = libvlc_media_get_state(m);
printf("media state %d\n", state);
                int seconds = (int)(ceil(libvlc_media_get_duration(m)/1000));
                printf(LOG_NAME "Media time %d\n", seconds);


libvlc_state_t state = libvlc_media_get_state(m);
printf("media state %d\n", state);

                if (sleep(seconds) != 0) {
                    printf(LOG_NAME "Play interrupted.");
                    libvlc_media_release (m);
                    exit(EXIT_FAILURE);
                }

                libvlc_media_release (m);

                if (p->more) { // loop forever
                    p = p->more;
                } else {
                    p = files;
                }
            }
    
            libvlc_media_player_stop (mp);
            libvlc_media_player_release (mp);
            libvlc_release (inst);
            free(fskiplist);
            free(folder);
            exit(EXIT_SUCCESS);
        default:
            printf(LOG_NAME "Playlist %s (pid=%d)\n", folder, pid);
            free(folder);
            break;
    }
    return pid;
}

static bool stop_vlc(int pid) {
    printf(LOG_NAME "Stop pid=%d\n", pid);
    if (kill(pid, SIGTERM) == -1) {
        perror("kill");
        return false;
    }
    int status;
    if (waitpid(pid, &status, WUNTRACED|WCONTINUED) == -1) {
        perror("waitpid");
        return false;
    }
    return true;
}


static bool cmd_load(int sock, const char const *msg) {
    playlist *playlists = get_dir(NULL, "", false);
    if (playlists) {
        playlist *p = playlists;
        char *response = "Loaded playlists\n";
        if (write(sock, response , strlen(response )) != strlen(response)) {
            perror("send");
        }
        char *path = get_path("", "");
        int pathlen = strlen(path) + 1; // +1 for slash
        free(path);
        while (p) {
            if (write(sock, p->folder + pathlen, strlen(p->folder) - pathlen) != strlen(p->folder) - pathlen) {
                perror("send");
            }
            if (write(sock, "\n", strlen("\n")) != strlen("\n")) { // separated by newlines
                perror("send");
            }
            p = p->more;
        }
    } else {
        char *response = "No playlists.\n";
        if (write(sock, response, strlen(response)) != strlen(response)) {
            perror("send");
        }
    }
    playlist_clear(playlists);
    return true;
}

static bool cmd_play(int sock, const char const *msg) {
    const char cmd[] = "play";

    const char *response = NULL;
    const char const *folder = NULL;
    if (msg + sizeof(cmd) != 0) {
        folder = msg + sizeof(cmd);
        response = folder;
    } else {
	if (vlc_pid != -1) {
            printf(LOG_NAME "Already playing\n");
            response = "Playing";
            folder = NULL;
	} else {
            folder = NULL;
            response = "Select playlist";
        }
    }

    if (folder) {
        if (vlc_pid != -1) {
            stop_vlc(vlc_pid);
        }
        vlc_pid = start_vlc(folder);
    }

    if (!response) {
        response = "Failed!";
    }
    if (write(sock, response, strlen(response)) != strlen(response)) {
        perror("send");
    }

    return vlc_pid != -1;
}

static bool cmd_stop(int sock, const char const *msg) {
    bool success = true;
    if (vlc_pid != -1) {
        if (!stop_vlc(vlc_pid)) {
            success = false;
            // should probably restart webbox-manager
        }
        vlc_pid = -1;
    }
    const char ok[] = "Stopped.";
    const char nok[] = "Stop failed!";
    const char *response = success ? ok : nok;
    if (send(sock, response, strlen(response), 0) != strlen(response)) {
        perror("send");
    }
    return true;
}

static bool cmd_skip(int sock, const char const *msg) {
    const char *response = NULL;

    if (vlc_pid == -1) {
        response = "Nothing playing.";
    } else {
        /*char *pf = get_path("", "", false);
        playlist *p = latest_get(playlists, pf);
        free(pf);*/
        //playlist *playlists = get_dir(NULL, "", false);
        //playlist *p = latest_get(playlists, "");
        char *latests = read_file(PLAYLIST_LATEST);
        if (latests) {
            char *latest = strtok(latests, "\n");
            char *filename = NULL; //get_path(p->folder, LATEST_FILENAME);
            if (latests) {
                char *latest = strtok(latests, "\n");
printf("latest2 %s\n", latest);
                //free(filename);
                if (latest) {
                    char *fskip = latest; //get_path("", latest);
                    if (fskip) {
printf("fskip %s\n", fskip);
                        if (access(fskip, F_OK ) == 0) {
                            FILE *file = fopen(PLAYLIST_SKIP, "at");
                            if (file) {
                                int length = strlen(fskip);
                                if (fwrite(fskip, 1, length, file) == length) {
                                    if (fwrite("\n", 1, 1, file) == 1) {
                                        printf(LOG_NAME "Skip %s: %s\n", PLAYLIST_SKIP, fskip);
                                        response = "Skipped";
                                    } else {
                                        printf(LOG_NAME "Skip failed %s\n", PLAYLIST_SKIP);
                                    }
                                } else {
                                    printf(LOG_NAME "Skip failed %s\n", PLAYLIST_SKIP);
                                }
                                fclose(file);
                                if (response) {
                                    if (vlc_pid != -1) {
                                        stop_vlc(vlc_pid);
                                        vlc_pid = -1;
                                    }
                                    vlc_pid = -1;
                                    //vlc_pid = start_vlc(p->folder);
                                }
                            }
                        } else {
                            perror("access");
                        }
                        //free(fskip);
                    }
                    //free(latest);
                }
            }
        }
        free(latests);
        //playlist_clear(playlists);
    }

    if (!response) {
        response = "Skip failed, check file write permissions.";
    }
    printf(LOG_NAME "%s\n", response);
    if (send(sock, response, strlen(response), 0) != strlen(response)) {
        perror("send");
    }
    return true;
}

static webbox_http_command commands[] = {
    { .name = "/load", .handle = cmd_load},
    { .name = "/play", .handle = cmd_play},
    { .name = "/stop", .handle = cmd_stop},
    { .name = "/skip", .handle = cmd_skip},
};

static bool command(int sock, const char const *path) {
    for (int i=0; i<sizeof(commands)/sizeof(commands[0]); i++) {
        if (strncmp(path, commands[i].name, strlen(commands[i].name)) == 0) {
            printf(LOG_NAME "%s\n", path);
            commands[i].handle(sock, path);
            return true;
        }
    }
    return false;
}

static void command_exit(void) {
    printf("command_exit\n");
}

webbox_http_command webbox_http_vlc = {
    .name = "/VLC",
    .handle = command,
    .exit = command_exit,
};
