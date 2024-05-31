#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

/* Функция для печати справки */
void print_help() {
    printf("Использование: программа [опции]\n");
    printf("Опции:\n");
    printf("  -h, --help            Печать этой справки\n");
    printf("  -v, --verbose         Включить подробный режим\n");
    printf("  -f, --file <file>     Указать имя файла\n");
}

int main(int argc, char *argv[]) {
    int opt;
    int verbose_flag = 0;
    char *file_name = NULL;

    /* Определение длинных опций */
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"verbose", no_argument, 0, 'v'},
        {"file", required_argument, 0, 'f'},
        {0, 0, 0, 0}
    };

    /* Обработка опций командной строки */
    while ((opt = getopt_long(argc, argv, "hvf:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'h':
                print_help();
                exit(EXIT_SUCCESS);
            case 'v':
                verbose_flag = 1;
                break;
            case 'f':
                file_name = optarg;
                break;
            default:
                print_help();
                exit(EXIT_FAILURE);
        }
    }
