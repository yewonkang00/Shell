#include <stdio.h>

#include <string.h>

#include <unistd.h>

#include <sys/wait.h>

#include <fcntl.h>

#define MAXARG 7

 

// Input Redirection 함수 

void redir_in(int back, char *argv[], char* input) {

    int fd;

    int status;

    pid_t pid;

    

    pid = fork();

 

    if(pid < 0) {

        perror("Fork Failed\n");

    }

 

     // 부모일 경우

    else if(pid > 0) {                          

        if(!back) pid = wait(&status);    // foreground process

                                        // 자식 프로세스가 종료될 때 까지 대기

        else {                           // background process

             printf("[1] %d\n", getpid());      // pid 출력

 

             //자식 프로세스가 종료될 때 까지 기다리지 않고 종료

             waitpid(pid, &status, WNOHANG);

 

        }

    }

 

    // 자식일 경우

    else {                                          

        if((fd = open(input, O_RDONLY)) == 1) { 

            perror(argv[0]);

            exit(2);

        }

        dup2(fd, 0);  // fd를 표준 입력으로 redirection

        close(fd); 

        execvp(argv[0], argv);  // argv 실행

    }

}

 

// Output Redirection 함수

void redir_out(int back, char *argv[], char* output) {

    int fd;

    int status;

    pid_t pid;

 

    pid = fork();

    if(pid < 0) {

        perror("Fork Failed\n");

    }

 

    // 부모일 경우

    else if(pid > 0) {                          

        if(!back) pid = wait(&status);    // foreground process

 

        else {                           // background process

             printf("[1] %d\n", getpid());

             waitpid(pid, &status, WNOHANG);

        }

    }

    

    // 자식일 경우

    else {

        fd = open(output, O_CREAT | O_TRUNC | O_WRONLY, 0600);

        dup2(fd, 1);            // fd를 표준 출력으로 redirection

        close(fd);

        execvp(argv[0], argv);  // argv 실행

    }

}

 

 

// Pipe 실행 함수

void pipeAct(int back, char* arg1[], char* arg2[]) {

    int fd[2];      // 2개의 fd를 담을 배열 정의

    int status;     // 자식 프로세스의 상태

    pid_t pid1;

    pid_t pid2;

    

    pid1 = fork();

    if(pid1 < 0) {

        perror("1st Fork Failed\n");

        exit(1);

    }

 

// 부모일 경우

    else if(pid1 > 0) {                

        if(!back) pid1 = wait(&status);    // foreground process

 

 

        else {                           // background process

             printf("[1] %d\n", getpid());

             waitpid(pid1, &status, WNOHANG);

        }

    }

 

    else {

        // pipe를 호출해 두개의 fd로 배열을 채워줌

        // fd[1] 에 쓰고 fd[0] 으로 읽어야함

 

        if(pipe(fd) == -1) {            

            perror("Pipe Failed\n");

            exit(1);

        }

       

        pid2 = fork();

        if(pid2 < 0) {

            perror("2nd Fork Failed\n");

            exit(1);

        }

        else if(pid2 == 0) {            // 자식 프로세스 일 경우

            dup2(fd[0], 1);             // fd[1]을 표준 출력으로 redirection

            close(fd[0]);               // fd[0]과 fd[1] 닫기

            close(fd[1]);

            execvp(arg1[0], arg1);      // arg1 실행

 

            perror("Parent Execvp Failed\n");

            exit(1);

        }

 

        else {                          // 부모 프로세스 일 경우


            dup2(fd[1], 0);             // fd[0]을 표준 입력으로 redirection

            close(fd[1]);               // fd[1]과 fd[0] 닫기
	    close(fd[0]);

            execvp(arg2[0], arg2);      // arg2 실행

 

            perror("Child Execvp Failed\n");

            exit(1);

        }

    }

}

 

 

// 연산자가 없는 일반 명령어 실행 함수

void execute(int back, char *argv[]) {

    int status;

    pid_t pid;

    

    pid = fork();

 

    if(pid < 0) {

        perror("Fork Failed\n");

    }

 

// 부모일 경우

    else if(pid > 0) {                

        if(!back) pid = wait(&status);    // foreground process

 

 

        else {                           // background process

             printf("[1] %d\n", getpid());

             waitpid(pid, &status, WNOHANG);

        }

    }

 

    else execvp(argv[0], argv);

}

 

 

// 명령어가 비어있는지 확인하는 함수

int checkCmd(char buf[]) {

    if(strcmp(buf, "") && strcmp(buf, "\t") && strcmp(buf, " "))

        return 0;

    return 1;

}

 

 

int main() {

    char cmd[256];          // 명령어 입력받을 배열

    char* args[MAXARG];     // 명령어 임시 저장

    char* arg[MAXARG];      // 연산자 명령어가 아닌 명령어 저장

    char* s;                // 구분자로 나눈 문자열

    char* saveptr;          // 다음 처리를 위한 위치를 저장하는 포인터

    pid_t pid;

    int status;            // 자식 프로세스 상태

    int num;

    int m;                  // <, >, |, & 연산자 명령어 표시할 번호

    int mark;               // 연산자 명령어가 있는 위치

 

    char *pipe_before[MAXARG];  //pipe 앞부분 명령어

    char *pipe_after[MAXARG];   //pipe 뒷부분 명령어

    char input[20] = "";        //redirection input 할 명령어

    char output[20] = "";       //redirection output 할 명령어

    int back = 0;               //backgroud process(&) 유무

    

    while(1) {

        printf("osh> ");

        fflush(stdout);          // printf 출력

        gets(cmd);              // 명령어 입력받음

 

        if(cmd != NULL && !checkCmd(cmd)) {

            args[0] = cmd;      // args 배열에 cmd 저장

            args[1] = (char *)0;

 

            // exit 입력 시 쉘 종료

            if(!strcmp(args[0], "exit")) {

                printf("Bye\n");

                exit(0);

            }

 

            num = 0;

            mark = 0;

            m = 0;

            back = 0;

            // args[0]에서 띄어쓰기, \t, \n 가 나오기 전까지의 문자열

            // saveptr 에 다음 위치 저장

            s = strtok_r(args[0], " \t\n", &saveptr);

                                                        

            // s가 null이 아닐 동안 실행

            while(s) {

               //s와 문자열 비교

                if(!strcmp(s, "|")) {

                    m = 1;

                    mark = num;    // pipe의 위치를 mark에 저장

 

                   // pipe 앞의 명령어를 pipe_before 배열에 저장

                    for (int i = 0; i < mark; i++) {

                        pipe_before[i] = arg[i];

                    }

                    pipe_before[mark] = (char *)0;    // 명령어 뒤에 null 처리

                }

 

                else if(!strcmp(s, "<")) {

                    m = 2;

                    mark = num;    // “<”의 위치를 mark에 저장

                }

 

                else if(!strcmp(s, ">")) {

                    m = 3;

                }

 

                else if(!strcmp(s, "&")) {

                    back = 1;      // “&”가 있을 경우 back = 1;

                }

                else {

                    if(m < 2) {

                       // s가 연산자 기호가 아닌 경우 arg 배열에 저장

                        arg[num] = s;       

                        num++;

                    }

 

                    else if(m == 2) {

                        // 앞에 < 를 만난 경우 input에 s 저장

                        strcpy(input, s);   

                    }

 

                    // 앞에 > 를 만난 경우 output에 s 저장

                    else strcpy(output, s); 

                }

 

                // 이후 띄어쓰기, \t, \n 가 나오기 전까지의 문자열

                s = strtok_r(NULL, " \t\n", &saveptr);  

            }

 

            arg[num] = (char *)0;

 

            if(m == 0) {          // 아무런 연산자 명령어가 없는 경우

                execute(back, arg); 

            }

 

            else if(m == 1) {      // pipe 명령어가 있는 경우

               // pipe_after에 pipe 뒷부분 명령어 저장

                for(int i = 0; i < num - mark; i++) {

                    pipe_after[i] = arg[i+mark];

                }

 

                pipe_after[num-mark] = (char *)0;   // 명령어 뒤에 null 처리

                pipeAct(back, pipe_before, pipe_after);

            }

      

             // Input Redirection 명령어가 있는 경우

            else if(m == 2) redir_in(back, arg, input);

 

             // Output Redirection 명령어가 있는 경우

            else if(m == 3) redir_out(back, arg, output);

        }

    }

} 
