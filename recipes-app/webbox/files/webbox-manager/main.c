#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/un.h>

#include "webbox_module.h"

static webbox_module *modules[] = {
//    &webbox_console,
//    &webbox_http,
    &webbox_gui,
};

static const char manager_socket_name[] = "\0WebboxManager";

static bool start_modules(void) {
    printf("%s\n", __func__);
    
    // modules share manager socket for callback
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr;
    memset(&addr, 0x0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    memcpy(addr.sun_path, manager_socket_name, sizeof(manager_socket_name));
    connect(fd, (struct sockaddr*)&addr, sizeof(addr));

    for (int i=0; i<sizeof(modules)/sizeof(modules[0]); i++) {
        webbox_module *module = modules[i];
        if (!webbox_module_process(module, fd)) {
            return false;
        }
    }

    return true;
}

static void stop_modules() {
    printf("%s\n", __func__);

    for (int i=sizeof(modules)/sizeof(modules[0])-1; i>=0; i--) {
        webbox_module_signal(modules[i]);
    }
}

static int create_manager_socket(void) {
    int fd;
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
      perror("socket");
      return -1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    memcpy(addr.sun_path, manager_socket_name, sizeof(manager_socket_name));
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
      perror("bind");
      return -1;
    }

    if (listen(fd, 1) == -1) {
      perror("listen");
      return -1;
    }

    return fd;
}

int main(int argc, char *argv[]) {
    printf("Type \"quit\" to exit %s.\n", manager_socket_name + 1);

    int manager_socket = create_manager_socket();
    if (manager_socket == -1) {
        exit(EXIT_FAILURE);
    }

    bool success = start_modules();

    bool quit = false;
    while (success && !quit) {
        int module_socket = accept(manager_socket, NULL, NULL);
        if (module_socket == -1) {
          perror("accept");
          continue;
        }

        while (!quit) {
            char buf[sizeof("quit")]; // maximum message length
            int n = read(module_socket,buf,sizeof(buf)-1);
            if (n > 0) {
                buf[n] = '\0';
                printf("WebboxManager read %u bytes %s\n", n, buf);
                if (strncmp(buf, "quit", sizeof("quit") - 1) == 0) {
                    quit = true;
                }
            } else { // modules share socket and never quit so -1 and 0 are both failures
                success = false;
                quit = true;
                if (n == -1) {
                    perror("WebboxManager read");
                }
            }
        }
        (void)close(module_socket);
    }

    stop_modules();

    close(manager_socket);

    printf("exit %s\n", manager_socket_name + 1);
    exit(success ? EXIT_SUCCESS : EXIT_FAILURE);
}
