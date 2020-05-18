#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <memory.h>

#define IDENTICAL 1
#define DIFFERENT 2
#define SIMILAR 3
#define ERROR -1

void close_files(int f1, int f2);

int run(char *argv1, char *argv2);

bool check_identical(int fptr1, int fptr2, int *size1, int *size2);

int main(int argc, char *argv[]) {
    return run(argv[1], argv[2]);
}

/**
 * close the files.
 * @param f1 - file descriptor of first file.
 * @param f2 - file descriptor of second file.
 */
void close_files(int f1, int f2) {
    close(f1);
    close(f2);
}

/**
 * check all the offsets of the files and compare characters.
 * @param argv1 - the name of the first file.
 * @param argv2 - the name of the second file.
 * @return 1 if the files are identical, 2 if the files are different,
 * and 3 if the files are similar.
 */
int run(char *argv1, char *argv2) {
    int fptr1, fptr2;
    // open first file
    fptr1 = open(argv1, O_RDONLY);
    // first file did not open
    if (fptr1 < 0) {
        write(2, "Error in system call\n", strlen("Error in system call\n"));
        exit(ERROR);
    }
    // open second file
    fptr2 = open(argv2, O_RDONLY);
    // second file did not open
    if (fptr2 < 0) {
        write(2, "Error in system call\n", strlen("Error in system call\n"));
        close(fptr1);
        exit(ERROR);
    }
    char c1, c2;
    int size_file1 = 0, size_file2 = 0;
    // check if identical
    bool identical = check_identical(fptr1, fptr2, &size_file1, &size_file2);
    if (identical) {
        return IDENTICAL;
    }
    if (size_file1 == 0 || size_file2 == 0) {
        return SIMILAR;
    }
    // save the min size for check if the files are similar
    double size_min_file = (size_file1 < size_file2) ? size_file1 : size_file2;
    // count the equals char of some offset of the files
    double count_equal;
    int offset2, offset1 = 0;
    int seek;
    while ((seek = lseek(fptr1, offset1, SEEK_SET)) != -1 && offset1 != size_file1) {
        offset1++;
        read(fptr1, &c1, 1);
        // start from the beginning of the second file
        if (lseek(fptr2, 0, SEEK_SET) == -1) {
            close_files(fptr1, fptr2);
            write(2, "Error in system call\n", strlen("Error in system call\n"));
            exit(ERROR);
        }
        offset2 = 0;
        while ((seek = lseek(fptr2, offset2, SEEK_SET)) != -1 && offset2 != size_file2) {
            read(fptr2, &c2, 1);
            offset2++;
            // if equal - check how much chars are equal in this offset
            if (c1 == c2) {
                count_equal = 1;
                while (read(fptr1, &c1, 1) > 0 && read(fptr2, &c2, 1) > 0) {
                    if (c1 == c2) {
                        count_equal++;
                    }
                }
                // if more than half chars are equals - the files are similar
                if (count_equal >= size_min_file / 2) {
                    close_files(fptr1, fptr2);
                    return SIMILAR;
                }
            }
        }
        if (seek == -1) {
            write(2, "Error in system call\n", strlen("Error in system call\n"));
            close_files(fptr1, fptr2);
            return ERROR;
        }
    }
    if (seek == -1) {
        write(2, "Error in system call\n", strlen("Error in system call\n"));
        close_files(fptr1, fptr2);
        return ERROR;
    }
    close_files(fptr1, fptr2);
    return DIFFERENT;
}

/**
 *
 * @param fptr1 - file descriptor of first file.
 * @param fptr2 - file descriptor of second file.
 * @param size1 - the size of the first file.
 * @param size2 - the size of the second file.
 * @return true if the files are identical and false otherwise.
 */
bool check_identical(int fptr1, int fptr2, int *size1, int *size2) {
    char c1, c2;
    bool identical = true;
    // check if the files are identical and count the size of the files
    while (read(fptr1, &c1, 1) > 0 && read(fptr2, &c2, 1) > 0) {
        if (c1 != c2) {
            identical = false;
        }
        (*size1)++;
        (*size2)++;
    }
    // we arrive to the end of the files
    if (read(fptr1, &c1, 1) == 0 && read(fptr2, &c2, 1) == 0) {
        // files are equal
        if (identical) {
            close_files(fptr1, fptr2);
            return true;
        }
    }
    // for size of the files
    while (read(fptr1, &c1, 1) > 0) {
        (*size1)++;
    }
    while (read(fptr2, &c2, 1) > 0) {
        (*size2)++;
    }
    return false;
}