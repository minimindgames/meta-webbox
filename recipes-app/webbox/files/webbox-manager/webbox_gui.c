#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "webbox_module.h"

extern void init_gbm_egl_gles(void);
extern void main_gbm(void);
extern void main_wayland(void);
extern void main_modeset(void);
extern void main_kms(void);

static bool process(int manager_socket) {
//init_gbm_egl_gles();
//main_gbm();

main_kms();
//main_modeset(); // good fb
//main_wayland(); // good
return 1;
    while (fcntl(STDIN_FILENO, F_GETFD) != -1 || errno != EBADF) {
        char buf[80];
        int rc = read(STDIN_FILENO, buf, sizeof(buf));
        int wc = write(manager_socket, buf, rc);
        if (rc != wc) {
            perror("write");
        }
    }
    return false;
}

static void signal_handler(int sig_no) {
    close(STDIN_FILENO);
}

webbox_module webbox_gui = {
    .name = __FILE__,
    .process = process,
    .signal_handler = signal_handler,
};
