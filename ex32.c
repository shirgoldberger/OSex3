#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <memory.h>
#include <wait.h>
#include <errno.h>
#include <stdbool.h>
#include <time.h>

#define SIZE 150
#define ERROR -1
#define NO_C_FILE_GRADE 0
#define NO_C_FILE_STR "NO_C_FILE"
#define COMPILATION_ERROR_GRADE 10
#define COMPILATION_ERROR_STR "COMPILATION_ERROR"
#define TIMEOUT_GRADE 20
#define TIMEOUT_STR "TIMEOUT"
#define WRONG_GRADE 50
#define WRONG_STR "WRONG"
#define SIMILAR_GRADE 75
#define SIMILAR_STR "SIMILAR"
#define EXCELLENT_GRADE 100
#define EXCELLENT_STR "EXCELLENT"

typedef struct student {
    // name of the directory of the student
    char name[SIZE];
    char comment[SIZE];
    int grade;
} student;

void read_lines(char *argv, char lines[3][SIZE]);

void check_lines(char lines[3][SIZE]);

void run(char *input, char *output, student *s, char *path);

void run_all_subdirs(char lines[3][150]);

bool is_compile(char path[], struct dirent *pDirent);

void check_files(char *path, char *input, char *output, student *s);

void save_student(student *s);

void compare_output(char *output, char *outputFile, student *s);

void delete_file(char *path);

int main(int argc, char *argv[]) {
    char lines[3][SIZE];
    read_lines(argv[1], lines);
    check_lines(lines);
    run_all_subdirs(lines);
    return 0;
}

/**
 * read the lines of the configuration file.
 * @param argv - the name of the configuration file.
 * @param lines - the array of the lines in the configuration file.
 * @return 0 if success to read the file and -1 otherwise.
 */
void read_lines(char *argv, char lines[3][SIZE]) {
    int fd = open(argv, O_RDONLY);
    if (fd < 0) {
        write(2, "error with open configuration file", strlen("error with open configuration file"));
        exit(ERROR);
    }
    char buffer[SIZE * 3];
    if (read(fd, buffer, sizeof(buffer)) == -1) {
        write(2, "error with read configuration file", strlen("error with open configuration file"));
        close(fd);
        exit(ERROR);
    }
    int num_line = 0;
    int index_buffer = 0, index_line = 0;
    while (num_line < 3) {
        // arrive to the end of the line
        if (buffer[index_buffer] == '\n') {
            lines[num_line][index_line] = '\0';
            index_line = 0;
            num_line++;
        } else {
            // save in lines according to line's number
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
    close(fd);
}

/**
 * check if the ines in the configuration file are valid.
 * @param lines - the array of the lines in the configuration file.
 */
void check_lines(char lines[3][SIZE]) {
    int fd1 = open(lines[1], O_RDONLY);
    int fd2 = open(lines[2], O_RDONLY);
    if (fd1 < 0 || fd2 < 0) {
        write(2, "Input/output File not exist\n", strlen("Input/output File not exist\n"));
        exit(-1);
    }
    // close files
    if (fd1 >= 0) {
        close(fd1);
    }
    if (fd2 >= 0) {
        close(fd2);
    }
}

/**
 * for each directory, save the name in struct of student
 * and send to check the student's files.
 * @param lines - the array of the lines in the configuration file.
 */
void run_all_subdirs(char lines[3][150]) {
    DIR *pDir;
    struct dirent *pDirent;
    // open the dir we got in the configuration file
    if ((pDir = opendir(lines[0])) == NULL) {
        write(2, "Not a valid directory\n", strlen("Not a valid directory\n"));
        exit(ERROR);
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
            // create the path to the student's directory
            char path[SIZE] = {0};
            strncpy(path, lines[0], strlen(lines[0]));
            strcat(path, "/");
            strcat(path, pDirent->d_name);
            check_files(path, lines[1], lines[2], &s);
            save_student(&s);
        }
    }
    closedir(pDir);
    delete_file("student.out");
}

/**
 * checks if there is a c file and send him to running.
 * @param path - path to the student's directory.
 * @param input - path to the input file in the configuration file.
 * @param output - path to the correct output file.
 * @param s - struct of the current student.
 */
void check_files(char *path, char *input, char *output, student *s) {
    struct dirent *pDirent;
    DIR *pDir;
    // open the dir of the student
    if ((pDir = opendir(path)) == NULL) {
        write(2, "can't open directory\n", strlen("can't open directory\n"));
        return;
    }
    bool has_CFile = false;
    while ((pDirent = readdir(pDir)) != NULL) {
        // check if this is a c file
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
    closedir(pDir);
}

/**
 * checks if we can compile the given file.
 * @param path - path to the student's directory.
 * @param pDirent - the c file.
 * @return true if the file is compile and false otherwise.
 */
bool is_compile(char path[], struct dirent *pDirent) {
    pid_t pid = fork();
    if (pid < 0) {
        write(2, "Error in system call\n", strlen("Error in system call\n"));
        return false;
    } else if (pid == 0) {
        // create the path of the c file
        char CFile_path[SIZE] = {0};
        strncpy(CFile_path, path, strlen(path));
        strcat(CFile_path, "/");
        strcat(CFile_path, pDirent->d_name);
        char *args[5] = {"gcc", "-o", "student.out", CFile_path, NULL};
        if (execvp("gcc", args) < 0) {
            write(2, "Error in system call\n", strlen("Error in system call\n"));
        }
        exit(EXIT_FAILURE);
    } else {
        int status;
        int wait = waitpid(pid, &status, 0);
        if (wait < 0) {
            write(2, "Error in system call\n", strlen("Error in system call\n"));
        }
        // not compile
        if (WEXITSTATUS(status) != 0) {
            return false;
            // compile
        } else {
            return true;
        }
    }
}

/**
 * run the program.
 * @param input - the input file to the student's program.
 * @param output - the name of the file that contains the correct output.
 * @param s - struct of student.
 * @param path - path to the student's directory.
 */
void run(char *input, char *output, student *s, char *path) {
    char outputFile[SIZE] = {0};
    strncpy(outputFile, path, strlen(path));
    strcat(outputFile, "/out.txt");
    time_t start, end;
    double dif;
    pid_t pid = fork();
    if (pid < 0) {
        write(2, "Error in system call\n", strlen("Error in system call\n"));
    } else if (pid == 0) {
        // get the input file
        int in = open(input, O_RDONLY);
        if (in < 0) {
            write(2, "Error in system call\n", strlen("Error in system call\n"));
            exit(EXIT_FAILURE);
        }
        // get the output file
        int out = open(outputFile, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
        if (out < 0) {
            close(in);
            write(2, "Error in system call\n", strlen("Error in system call\n"));
            exit(EXIT_FAILURE);
        }
        if (dup2(in, STDIN_FILENO) == -1) {
            write(2, "Error in system call\n", strlen("Error in system call\n"));
            close(in);
            close(out);
            exit(EXIT_FAILURE);
        }
        if (dup2(out, STDOUT_FILENO) == -1) {
            write(2, "Error in system call\n", strlen("Error in system call\n"));
            close(in);
            close(out);
            exit(EXIT_FAILURE);
        }
        char *args[2] = {"./student.out", NULL};
        if (execvp("./student.out", args) < 0) {
            write(2, "Error in system call\n", strlen("Error in system call\n"));
        }
        close(in);
        close(out);
        exit(EXIT_FAILURE);
    } else {
        int status;
        time(&start);
        int wait = waitpid(pid, &status, 0);
        if (wait < 0) {
            write(2, "Error in system call\n", strlen("Error in system call\n"));
            return;
        }
        time(&end);
        dif = difftime(end, start);
        // check time
        if (dif > 3) {
            s->grade = TIMEOUT_GRADE;
            strcpy(s->comment, TIMEOUT_STR);
        } else {
            // check output with ex31
            compare_output(output, outputFile, s);
        }
        // delete output file
        delete_file(outputFile);
    }
}

/**
 * run ex31.c to compare the student's output to the correct output and save
 * his grade according to the returned value.
 * @param output - the name of the file that contains the correct output.
 * @param outputFile - the name of the file that contains the student's output.
 * @param s - struct of student.
 */
void compare_output(char *output, char *outputFile, student *s) {
    int score;
    pid_t pid = fork();
    if (pid < 0) {
        write(2, "Error in system call\n", strlen("Error in system call\n"));
    } else if (pid == 0) {
        // run ex31.c
        char *args[] = {"./comp.out", output, outputFile, NULL};
        if (execvp("./comp.out", args) < 0) {
            write(2, "Error in system call\n", strlen("Error in system call\n"));
        }
        exit(EXIT_FAILURE);
    } else {
        int status = 0;
        int wait = waitpid(pid, &status, 0);
        if (wait < 0) {
            write(2, "Error in system call\n", strlen("Error in system call\n"));
            return;
        }
        // if compilation error not success
        if (WEXITSTATUS(status) == 0) {
            return;
        } else {
            score = WEXITSTATUS(status);
        }

        if (score == 1) { // identical
            s->grade = EXCELLENT_GRADE;
            strcpy(s->comment, EXCELLENT_STR);
        } else if (score == 2) { // different
            s->grade = WRONG_GRADE;
            strcpy(s->comment, WRONG_STR);
        } else if (score == 3) { // similar
            s->grade = SIMILAR_GRADE;
            strcpy(s->comment, SIMILAR_STR);
        }
    }
}

/**
 * save the students in the file 'results.csv'.
 * @param s - struct of student.
 */
void save_student(student *s) {
    if (strcmp(s->comment, "") == 0) {
        return;
    }
    int results = open("results.csv", O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
    if (results < 0) {
        write(2, "Error in system call\n", strlen("Error in system call\n"));
        return;
    }
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
    if (in < 0) {
        write(2, "Error in system call\n", strlen("Error in system call\n"));
        return;
    }
    close(results);
}

/**
 * delete file.
 * @param path - the path of the file we want to delete.
 */
void delete_file(char *path) {
    int status = 0;
    pid_t pid = fork();
    if (pid < 0) {
        write(2, "Error in system call\n", strlen("Error in system call\n"));
    } else if (pid == 0) {
        char *args[4] = {"rm", "-rf", path, NULL};
        if (execvp("rm", args) < 0) {
            write(2, "Error in system call\n", strlen("Error in system call\n"));
        }
    } else {
        int wait = waitpid(pid, &status, 0);
        if (wait < 0) {
            write(2, "Error in system call\n", strlen("Error in system call\n"));
        }
    }
}