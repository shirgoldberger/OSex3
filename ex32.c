#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <memory.h>
#include <wait.h>
#include <errno.h>
#include <stdbool.h>

#define ERROR_MESSAGE fprintf(stderr, "Error in system call\n");
#define CD_ERROR_MESSAGE fprintf(stderr, "Error: No such file or directory\n");
#define SIZE 150
#define NO_C_FILE_GRADE 0
#define NO_C_FILE_STR "NO_C_FILE"
#define COMPILATION_ERROR_GRADE 10
#define COMPILATION_ERROR_STR "COMPILATION_ERROR"

typedef struct student {
    char name[SIZE];
    char comment[SIZE];
    int grade;
} student;

int read_lines(char *argv, char lines[3][150]);

void run(char *input, char *output, student *s, char *path);

void run_all_subdirs(char lines[3][150]);

bool is_compile(char path[], struct dirent *pDirent);

void check_files(char *path, char *input, char *output, student *s);

void save_student(student *s);

int main(int argc, char *argv[]) {
    char lines[3][SIZE];
    if (read_lines(argv[1], lines) == -1) {
        return -1;
    }
    run_all_subdirs(lines);
}
/**
 * @param argv - the name of the configuration file.
 * @param lines - the array of the lines in the configuration file.
 * @return 0 if success to read the file and -1 otherwise.
 */
int read_lines(char *argv, char lines[3][150]) {
    int fd = open(argv, O_RDONLY);
    char buffer[SIZE * 3];
    if (read(fd, buffer, sizeof(buffer)) == -1) {
        return -1;
    }
    int num_line = 0;
    int index_buffer = 0, index_line = 0;
    while (num_line < 3) {
        if (buffer[index_buffer] == '\n') {
            lines[num_line][index_line] = '\0';
            index_line = 0;
            num_line++;
        } else {
            switch (num_line) {
                case 0:
                    lines[0][index_line] = buffer[index_buffer];
                    break;
                case 1:
                    lines[1][index_line] = buffer[index_buffer];
                    break;
                case 2:
                    lines[2][index_line] = buffer[index_buffer];
                    break;
            }
            index_line++;
        }
        index_buffer++;
    }
    return 0;
}

void run_all_subdirs(char lines[3][150]) {
    DIR *pDir;
    struct dirent *pDirent;
    // open the dir we got in the configuration file
    if ((pDir = opendir(lines[0])) == NULL) {
        exit(1);
    }
    while ((pDirent = readdir(pDir)) != NULL) {
        if (!strcmp(pDirent->d_name, "..") || !strcmp(pDirent->d_name, ".")) {
            if (pDirent == NULL && errno != 0) {
                exit(EXIT_FAILURE);
            }
            continue;
        }
        // it is a directory
        if (pDirent->d_type == DT_DIR) {
            // create student
            student s = {0};
            strcpy(s.name, pDirent->d_name);
            char path[SIZE] = {0};
            strncpy(path, lines[0], strlen(lines[0]));
            strcat(path, "/");
            strcat(path, pDirent->d_name);
            check_files(path, lines[1], lines[2], &s);
            save_student(&s);
        }
    }
    closedir(pDir);
}

void check_files(char *path, char *input, char *output, student *s) {
    struct dirent *pDirent;
    DIR *dirc = opendir(path);
    if (dirc == NULL) {
        exit(EXIT_FAILURE);
    }
    bool has_CFile = false;
    while ((pDirent = readdir(dirc)) != NULL) {
        if (strcmp(pDirent->d_name + (strlen(pDirent->d_name)) - 2, ".c") == 0) {
            has_CFile = true;
            bool isCompileFile = is_compile(path, pDirent);
            // not compile
            if (!isCompileFile) {
                s->grade = COMPILATION_ERROR_GRADE;
                strcpy(s->comment, COMPILATION_ERROR_STR);
            } else {
                run(input, output, s, path);
            }
        }
    }
    if (!has_CFile) {
        s->grade = NO_C_FILE_GRADE;
        strcpy(s->comment, NO_C_FILE_STR);
    }
}

bool is_compile(char *path, struct dirent *pDirent) {
    pid_t pid = fork();

    char buffCFile[SIZE] = {0};
    strncpy(buffCFile, path, strlen(path));
    strcat(buffCFile, "/");
    strcat(buffCFile, pDirent->d_name);

    if (pid == 0) {
        char outputFile[SIZE] = {0};
        strncpy(outputFile, path, strlen(path));
        strcat(outputFile, "/a.out");
        char *command[5] = {"gcc", "-o", outputFile, buffCFile};
        execvp("gcc", command);
        exit(EXIT_FAILURE);
    } else {
        int status;
        waitpid(pid, &status, 0);
        if (WEXITSTATUS(status) != 0) {
            return false;
        } else {
            return true;
        }
    }
}

void run(char *input, char *output, student *s, char *path) {
    char exec_file[SIZE];
    strncpy(exec_file, path, strlen(path));
    strcat(exec_file, "/a.out");


    pid_t pid = fork();
    if (pid == 0) {
        char outputFile[SIZE];
        char* args[] = {"a.out", output, outputFile, NULL};
        execvp(exec_file ,args);
        exit(EXIT_FAILURE);
    } else {
        int status;
        waitpid(pid, &status, 0);

    }
    // check time
    // check output with ex31
}

void save_student(student *s) {
    int results = open("results.csv", O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
    char output[SIZE] = {0};
    // name
    strcat(output, s->name);
    strcat(output, ",");
    // grade
    char grade[sizeof(int)];
    sprintf(grade, "%d", s->grade);
    strcat(output, grade);
    strcat(output, ",");
    // description
    strcat(output, s->comment);
    strcat(output, "\n");
    // write to file
    int in = write(results, output, strlen(output));
}