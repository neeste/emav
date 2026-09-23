#include <stdio.h>
#include <stdlib.h>

int main() {
    FILE *fp = popen("osascript -e 'POSIX path of (choose folder with prompt \"Select Working Directory:\")' 2>/dev/null", "r");
    if (fp) {
        char path[1024];
        if (fgets(path, sizeof(path), fp) != NULL) {
            printf("Selected: %s\n", path);
        }
        pclose(fp);
    }
    return 0;
}
