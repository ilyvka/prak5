#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/wait.h>

#define ROUNDS 10

typedef struct {
    int round_num;
    int max_number;
    int is_thinker;
} RoundInfo;

void play_round(int read_fd, int write_fd, RoundInfo info) {
    if (info.is_thinker) {
        // �������-������������
        int target = rand() % info.max_number + 1;
        printf("����� %d: �������� ����� �� 1 �� %d\n", info.round_num, info.max_number);
        fflush(stdout);  // ������������ ����������� �����

        while (1) {
            int guess;
            read(read_fd, &guess, sizeof(guess));

            if (guess == target) {
                printf("����� %d: ����� %d ������� �� %d �������\n",
                    info.round_num, guess, guess);
                fflush(stdout);
                int response = 1;
                write(write_fd, &response, sizeof(response));
                break;
            }
            else {
                int response = 0;
                write(write_fd, &response, sizeof(response));
            }
        }
    }
    else {
        // �������-�����������
        for (int guess = 1; guess <= info.max_number; guess++) {
            write(write_fd, &guess, sizeof(guess));

            int response;
            read(read_fd, &response, sizeof(response));

            if (response == 1) break;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <max_number>\n", argv[0]);
        return 1;
    }

    int max_number = atoi(argv[1]);
    if (max_number <= 1) {
        fprintf(stderr, "Number must be greater than 1\n");
        return 1;
    }

    srand(time(NULL));

    // ������ ��� ������ � �������� �������
    int pipes_even[2];    // ��� ������ �������
    int pipes_odd[2];     // ��� �������� �������

    if (pipe(pipes_even) || pipe(pipes_odd)) {
        perror("pipe");
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    for (int round = 1; round <= ROUNDS; round++) {
        RoundInfo info = { round, max_number, round % 2 };

        if (pid == 0) { // �������� �������
            if (round % 2) {
                // �������� ����� - ������� ���������
                close(pipes_odd[1]);
                close(pipes_even[0]);
                play_round(pipes_odd[0], pipes_even[1], info);
            }
            else {
                // ������ ����� - ������� ����������
                close(pipes_odd[0]);
                close(pipes_even[1]);
                info.is_thinker = 1;
                play_round(pipes_even[0], pipes_odd[1], info);
            }
        }
        else { // ������������ �������
            if (round % 2) {
                // �������� ����� - �������� ����������
                close(pipes_odd[0]);
                close(pipes_even[1]);
                info.is_thinker = 1;
                play_round(pipes_even[0], pipes_odd[1], info);
            }
            else {
                // ������ ����� - �������� ���������
                close(pipes_odd[1]);
                close(pipes_even[0]);
                info.is_thinker = 0;
                play_round(pipes_odd[0], pipes_even[1], info);
            }
        }
    }

    if (pid > 0) {
        close(pipes_odd[0]);
        close(pipes_odd[1]);
        close(pipes_even[0]);
        close(pipes_even[1]);
        wait(NULL);
    }

    return 0;
}