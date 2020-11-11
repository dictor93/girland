#include "fcntl.h"
#include "unistd.h"

#include "spiffs.h"
#include "esp_spiffs.h"

/**
 * This example shows the default SPIFFS configuration when SPIFFS is
 * configured in compile-time (SPIFFS_SINGLETON = 1).
 *
 * To configure SPIFFS in run-time uncomment SPIFFS_SINGLETON in the Makefile
 * and replace the commented esp_spiffs_init in the code below.
 *
 */

void Fs_readFile(char*name, char*extBuf, int buffLen)
{
    int fd = open(name, O_RDONLY);
    if (fd < 0) {
        printf("Error opening file\n");
        return;
    }

    int read_bytes = read(fd, extBuf, buffLen);
    extBuf[read_bytes] = '\0';    // zero terminate string
    close(fd);
}

void Fs_writeFile(char*name, char*file)
{
    remove(name);

    int fd = open(name, O_WRONLY|O_CREAT, 0);
    if (fd < 0) {
        printf("Error opening file\n");
        return;
    }
    
    int written = write(fd, file, strlen(file));
    printf("Written %d bytes\n", written);

    close(fd);
}

static void example_fs_info()
{
    uint32_t total, used;
    SPIFFS_info(&fs, &total, &used);
    printf("Total: %d bytes, used: %d bytes", total, used);
}

void Fs_init(void)
{
    esp_spiffs_init();

    if (esp_spiffs_mount() != SPIFFS_OK) {
        printf("Error mount SPIFFS\n");
    }
}