#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <getopt.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

static int running = 0;
static int delay = 1;
static int counter = 0;
static char *conf_file_name = NULL;
static char *pid_file_name = NULL;
static int pid_fd = -1;
static char *app_name = NULL;
static FILE *log_stream;

/**
 * \brief Чтение конфигурации из файла конфигурации
 */
int read_conf_file(int reload)
{
	FILE *conf_file = NULL;
	int ret = -1;

	if (conf_file_name == NULL) return 0;

	conf_file = fopen(conf_file_name, "r");

	if (conf_file == NULL) {
		syslog(LOG_ERR, "Невозможно открыть файл конфигурации: %s, ошибка: %s",
				conf_file_name, strerror(errno));
		return -1;
	}

	ret = fscanf(conf_file, "%d", &delay);

	if (ret > 0) {
		if (reload == 1) {
			syslog(LOG_INFO, "Перезагружен файл конфигурации %s для %s",
				conf_file_name,
				app_name);
		} else {
			syslog(LOG_INFO, "Конфигурация %s прочитана из файла %s",
				app_name,
				conf_file_name);
		}
	}

	fclose(conf_file);

	return ret;
}

/**
 * \brief Функция для проверки файла конфигурации
 */
int test_conf_file(char *_conf_file_name)
{
	FILE *conf_file = NULL;
	int ret = -1;

	conf_file = fopen(_conf_file_name, "r");

	if (conf_file == NULL) {
		fprintf(stderr, "Не удается прочитать файл конфигурации %s\n",
			_conf_file_name);
		return EXIT_FAILURE;
	}

	ret = fscanf(conf_file, "%d", &delay);

	if (ret <= 0) {
		fprintf(stderr, "Неправильный файл конфигурации %s\n",
			_conf_file_name);
	}

	fclose(conf_file);

	if (ret > 0)
		return EXIT_SUCCESS;
	else
		return EXIT_FAILURE;
}

/**
 * \brief Обработчик сигналов.
 * \param	sig	идентификатор сигнала
 */
void handle_signal(int sig)
{
	if (sig == SIGINT) {
		fprintf(log_stream, "Отладка: остановка демона ...\n");
		/* Разблокировка и закрытие файла блокировки */
		if (pid_fd != -1) {
			lockf(pid_fd, F_ULOCK, 0);
			close(pid_fd);
		}
		/* Попытка удалить файл блокировки */
		if (pid_file_name != NULL) {
			unlink(pid_file_name);
		}
		running = 0;
		/* Сброс обработки сигналов на поведение по умолчанию */
		signal(SIGINT, SIG_DFL);
	} else if (sig == SIGHUP) {
		fprintf(log_stream, "Отладка: перезагрузка файла конфигурации демона ...\n");
		read_conf_file(1);
	} else if (sig == SIGCHLD) {
		fprintf(log_stream, "Отладка: получен сигнал SIGCHLD\n");
	}
}

/**
 * \brief Функция для демонизации приложения
 */
static void daemonize()
{
	pid_t pid = 0;
	int fd;

	/* Создание дочернего процесса */
	pid = fork();

	/* Произошла ошибка */
	if (pid < 0) {
		exit(EXIT_FAILURE);
	}

	/* Успех: Родительский процесс завершается */
	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	/* На успех: Дочерний процесс становится лидером сессии */
	if (setsid() < 0) {
		exit(EXIT_FAILURE);
	}

	/* Игнорировать сигнал, посылаемый от дочернего процесса к родительскому */
	signal(SIGCHLD, SIG_IGN);

	/* Создание второго дочернего процесса */
	pid = fork();

	/* Произошла ошибка */
	if (pid < 0) {
		exit(EXIT_FAILURE);
	}

	/* Успех: Родительский процесс завершается */
	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	/* Установка новых прав доступа к файлам */
	umask(0);

	/* Изменение рабочего каталога на корневой */
	/* или другой подходящий каталог */
	chdir("/");

	/* Закрытие всех открытых файловых дескрипторов */
	for (fd = sysconf(_SC_OPEN_MAX); fd > 0; fd--) {
		close(fd);
	}

	/* Повторное открытие stdin (fd = 0), stdout (fd = 1), stderr (fd = 2) */
	stdin = fopen("/dev/null", "r");
	stdout = fopen("/dev/null", "w+");
	stderr = fopen("/dev/null", "w+");

	/* Попытка записать PID демона в файл блокировки */
	if (pid_file_name != NULL)
	{
		char str[256];
		pid_fd = open(pid_file_name, O_RDWR|O_CREAT, 0640);
		if (pid_fd < 0) {
			/* Невозможно открыть файл блокировки */
			exit(EXIT_FAILURE);
		}
		if (lockf(pid_fd, F_TLOCK, 0) < 0) {
			/* Невозможно заблокировать файл */
			exit(EXIT_FAILURE);
		}
		/* Получение текущего PID */
		sprintf(str, "%d\n", getpid());
		/* Запись PID в файл блокировки */
		write(pid_fd, str, strlen(str));
	}
}

/**
 * \brief Печать справки для этого приложения
 */
void print_help(void)
{
	printf("\n Использование: %s [ОПЦИИ]\n\n", app_name);
	printf("  Опции:\n");
	printf("   -h --help                 Печать этой справки\n");
	printf("   -c --conf_file filename   Чтение конфигурации из файла\n");
	printf("   -t --test_conf filename   Проверка файла конфигурации\n");
	printf("   -l --log_file  filename   Запись логов в файл\n");
	printf("   -d --daemon               Демонизация этого приложения\n");
	printf("   -p --pid_file  filename   Файл PID, используемый демонизированным приложением\n");
	printf("\n");
}
int main(int argc, char *argv[])
{
	static struct option long_options[] = {
		{"conf_file", required_argument, 0, 'c'},
		{"test_conf", required_argument, 0, 't'},
		{"log_file", required_argument, 0, 'l'},
		{"help", no_argument, 0, 'h'},
		{"daemon", no_argument, 0, 'd'},
		{"pid_file", required_argument, 0, 'p'},
		{NULL, 0, 0, 0}
	};
	int value, option_index = 0, ret;
	char *log_file_name = NULL;
	int start_daemonized = 0;

	app_name = argv[0];

	/* Обработка всех аргументов командной строки */
	while ((value = getopt_long(argc, argv, "c:l:t:p:dh", long_options, &option_index)) != -1) {
		switch (value) {
			case 'c':
				conf_file_name = strdup(optarg);
				break;
			case 'l':
				log_file_name = strdup(optarg);
				break;
			case 'p':
				pid_file_name = strdup(optarg);
				break;
			case 't':
				return test_conf_file(optarg);
			case 'd':
				start_daemonized = 1;
				break;
			case 'h':
				print_help
