#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

extern int ar_dev_name(int di, char *name, int len);
extern int _ar_io_open(int di);

void* bg_thread(void* arg) {
    char buf[80];
    int res1 = _ar_io_open(6);
    printf("open res = %d\n", res1);
    int res = ar_dev_name(6, buf, 80);
    printf("ar_dev_name(6) = %d, name = %s\n", res, buf);
    return NULL;
}

int main() {
    pthread_t th;
    pthread_create(&th, NULL, bg_thread, NULL);
    pthread_join(th, NULL);
    return 0;
}
