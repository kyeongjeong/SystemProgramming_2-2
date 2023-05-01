///////////////////////////////////////////////////////////////////////////////////////
// File Name    : 2021202078_html_ls.c                                               //
// Date         : 2023/04/19                                                         //
// OS           : Ubuntu 16.04.5 Desktop 64bits                                      //
// Author       : Choi Kyeong Jeong                                                  //
// Student ID   : 2021202078                                                         //
// --------------------------------------------------------------------------------- //
// Title        : System Programming Assignment 2-1                                  //
// Descriptions : To output the results of Assignment 1-3 to an HTML file, with file //
//                names linked and color differentiation based on file type, in a    //
//                table format.                                                      //
///////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <ctype.h>
#include <fnmatch.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define MAX_LENGTH 1000
#define TITLE_HTML "html_ls.html"
#define URL_LEN 256
#define BUFSIZE 1024
#define PORTNO 40000

void listDirFiles(int a_hidden, int l_format, int S_size, int r_reverse, int h_readable, char* filename, FILE *file);
void printFileInfo(char* fileName, FILE *file, int l_format, int h_readable);
void getAbsolutePath(char *inputPath, char *absolutePath);
void joinPathAndFileName(char* path, char* Apath, char* fileName);
void sortByNameInAscii(char **fileList, int fileNum, int start, int r_reverse);
void sortByFileSize(char **fileList, char * dirPath, int fileNum, int start, int r_reverse);
void printPermissions(mode_t mode, FILE *file);
void printType(struct stat fileStat, FILE *file);
void findColor(char* fileName, char* color);
void printAttributes(struct stat fileStat, FILE *file, int h_readable, char *color);
int compareStringUpper(char* fileName1, char* fileName2);
int wildcard(FILE *file, char* patternName, int isPrint);
int loadHTML();

///////////////////////////////////////////////////////////////////////////////////////
// main                                                                              //
// --------------------------------------------------------------------------------- //
// Input: char* argv[] -> Directory name or file name or option that the user enters //
//        int argc -> The number of arguments and options                            //
// output:                                                                           //
// purpose: This program distinguishes between options and arguments provided by the //
//          user, identifies the type of option, and determines whether the argument //
//          and sorts the directories alphabetically.                                //
//          is a file or directory. It checks whether the file or directory exists   //
///////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[]) {
    
    DIR *dirp; //DIR 포인터
    struct dirent *dir; //dirent 구조체
    struct stat st; //파일 속성 
    int a_hidden = 0, l_format = 0, S_size = 0, r_reverse = 0, h_readable = 0;  // 각 옵션 포함 여부(0: 미포함, 1: 포함)
    int opt; //옵션 정보
    int isNotWild[MAX_LENGTH] = {0, }; //와일드카드 포함 여부(0: 포함, 1: 미포함)
    int sortflag = 0; //정렬 교정을 위한 임시 변수
    char Apath[MAX_LENGTH] = {'\0', }; //working directory의 절대 경로
    char title[MAX_LENGTH] = "html_ls.html"; // html 문서

    FILE *file = fopen(TITLE_HTML, "w"); //html 파일 생성 및 오픈

    fprintf(file, "<!DOCTYPE html>\n<html>\n<head>\n"); //title 생성
    getAbsolutePath(title, Apath); //html 문서의 절대 경로 받아오기
    fprintf(file, "<title>%s</title>\n</head>\n<body>\n", Apath); //절대경로로 title 설정

    while((opt = getopt(argc, argv, "alhSr")) != -1) { //옵션 정보 판별하기
        switch (opt)
        {
        case 'l': //옵션에 -l 포함한 경우
            l_format = 1; //l 플래그 세우기
            break;
        case 'a': //옵션에 -a 포함한 경우
            a_hidden = 1; //a 플래그 세우기
            break;
        case 'S': //옵션에 -S 포함한 경우
            S_size = 1; //S 플래그 세우기
            break;
        case 'r': //옵션에 -r 포함한 경우
            r_reverse = 1; //r 플래그 세우기
            break;
        case 'h': //옵션에 -h 포함한 경우
            h_readable = 1; //h 플래그 세우기
        case '?': //unknown
            break;
        default: //defalut
            break;
        }
    }

    if((a_hidden == 0) && (l_format == 0) && (S_size == 0) && (r_reverse == 0) && (h_readable == 0)) //옵션이 없는 경우
        optind = 0;

    if(optind >= 2) //-al, -la의 경우에도 optind는 1
        optind = 1;

    fprintf(file, "<h1>Welcome to System Programming Http</h1>\n<br>\n"); //header 작성

    if((argc-optind) == 1) { //인자가 없는 경우(옵션은 있을수도, 없을수도 있다)
        char currentPath[10] = "."; //현재 경로
        listDirFiles(a_hidden, l_format, S_size, r_reverse, h_readable, currentPath, file); //현재 디렉토리 하위 파일 출력
    }

    else { //인자로 경로를 한 개 이상 받은 경우
        for(int i = optind; i < argc; i++) { 
            //1. 디렉토리가 아님 2. 파일 안열림 3. 옵션이 아님 => 와일드카드거나, 존재하지 않거나
            if ((opendir(argv[i]) == NULL) && (stat(argv[i], &st)==-1) && (argv[i][0] != '-')) { 
                
                if(wildcard(file, argv[i], 0) == 0) { //와일드카드가 아니라면 => 존재하지 않는 파일
                    fprintf(file, "<b>cannot access %s: No such file or directory</b><br>\n", argv[i]); //error 문구 출력
                    isNotWild[i] = 1; //와일드카드가 아님을 저장
                }
                else //와일드카드라면
                    sortflag = 1; //sortflag 지정
            }
        }

        fprintf(file, "<br>\n");
        sortByNameInAscii(argv, argc, (optind + sortflag), r_reverse); //파일 및 디렉토리들을 아스키코드순으로 정렬

        for (int i = optind; i < argc; i++) { //인자 수만큼 반복
            if ((opendir(argv[i]) == NULL) && (stat(argv[i], &st) == -1) && (argv[i][0] != '-')) { //1. 디렉토리가 아님 2. 파일 안열림 3. 옵션이 아님 => 와일드카드거나, 존재하지 않거나
                if (isNotWild[i] == 0) //와일드카드라면
                    wildcard(file, argv[i], 1); //와일드카드 출력
                else
                    continue; //아니라면 건너뛰기
            }
        }

        char path[MAX_LENGTH]; //경로를 받아올 변수
        getcwd(path, MAX_LENGTH); //현재 경로 받아오기

        if (S_size == 1) { //-S 옵션이 있는 경우(사이즈별로 출력)

            char* temp[MAX_LENGTH]; //argv 배열의 내용을 복사해 파일 크기로 정렬할 배열
            for (int i = 0; i < argc; i++) //인자 수만큼 반복하면서
                temp[i] = argv[i]; //temp에 argv 내용 복사

            sortByFileSize(temp, path, argc, optind, r_reverse); //파일 사이즈별로 정렬

            for (int i = optind; i < argc; i++) { //인자 수만큼 반복

                // 1. 존재하는 파일임(옵션 x) 2. 디렉토리가 아님 3. run파일이 아님 => 파일정보 출력
                if ((stat(temp[i], &st) != -1) && (!S_ISDIR(st.st_mode)) && (i != 0) && (strcmp(temp[i], TITLE_HTML)!=0)) 
                    printFileInfo(temp[i], file, l_format, h_readable); //파일 정보 출력 함수
            }
            fprintf(file, "</table><br>\n"); //테이블 닫기
        }

        else{ //-S 옵션이 없는경우(아스키코드순으로 출력)

            for (int i = optind; i < argc; i++) { //인자 수만큼 반복
                
                // 1. 존재하는 파일임(옵션 x) 2. 디렉토리가 아님 3. run파일이 아님 => 파일정보 출력
                if ((stat(argv[i], &st) != -1) && (!S_ISDIR(st.st_mode)) && (i != 0) && (strcmp(argv[i], TITLE_HTML)!=0))
                    printFileInfo(argv[i], file, l_format, h_readable); // 파일정보 출력                
            }
            fprintf(file, "</table><br>\n"); //테이블 닫기
        }

        for(int i = optind; i < argc; i++) { //인자 수만큼 반복
            if((stat(argv[i], &st) != -1) && S_ISDIR(st.st_mode)) { //열리는 디렉토리인 경우

                listDirFiles(a_hidden, l_format, S_size, r_reverse, h_readable, argv[i], file); //디렉토리 하위 파일정보 출력
                if(l_format == 0) //-l 옵션이 없는 경우
                    fprintf(file, "<br>\n"); //입력받은 경로 간 줄바꿈                    
            }
        }
    }

    fprintf(file, "</body></html>");
    closedir; //디렉토리 close
    fclose(file);

    loadHTML();
    return 0; //프로그램 종료
}

int loadHTML() {
    
    struct sockaddr_in server_addr, client_addr;
    int socket_fd, client_fd;
    int len_out;
    unsigned int len;
    int opt = 1;
    int count = 0;
    char ch;

    FILE *file = fopen(TITLE_HTML, "r");   
    while((ch = fgetc(file)) != EOF)     
        count++;

    char *response_message = (char*) malloc(sizeof(char) * (count+1));

    rewind(file);
    fread(response_message, sizeof(char), count+1, file);
    fclose(file);

    if((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Server : Can't open stream socket\n");
        return 0;
    }

    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORTNO);

    if(bind(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Server : Can't bind local address\n");
        return 0;
    }

    listen(socket_fd, 5);

    while(1) {
        
        struct in_addr inet_clinet_address;
        char response_header[BUFSIZE] = {0, };
        char buf[BUFSIZE] = {0, };
        char tmp[BUFSIZE] = {0, };
        char url[URL_LEN] = {0, };
        char method[20] = {0, };
        char* tok = NULL;

        len = sizeof(client_addr);
        client_fd = accept(socket_fd, (struct sockaddr*)&client_addr, &len);
        if(client_fd < 0) {
            printf("Server : accept failed\n");
            return 0;
        }

        inet_clinet_address.s_addr = client_addr.sin_addr.s_addr;
        printf("[%s : %d] client was connected\n", inet_ntoa(inet_clinet_address), client_addr.sin_port);
        read(client_fd, buf, BUFSIZE);
        strcpy(tmp, buf);
        puts("=============================================");
        printf("Request from [%s : %d]\n", inet_ntoa(inet_clinet_address), client_addr.sin_port);
        puts(buf);
        puts("=============================================");

        tok = strtok(tmp, " ");
        strcpy(method, tok);
        if(strcmp(method, "GET") == 0) {
            tok = strtok(NULL, " ");
            strcpy(url, tok);
        }

        sprintf(response_header, "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n");
        write(client_fd, response_header, strlen(response_header));
        write(client_fd, response_message, strlen(response_message));
        
        printf("[%s : %d] client was disconnected\n", inet_ntoa(inet_clinet_address), client_addr.sin_port);
        close(client_fd);
    }
    close(socket_fd);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////
// wildcard                                                                          //
// --------------------------------------------------------------------------------- //
// Input: char* patternName -> The wildcard pattern to be checked                    //
//        int isPrint -> Determination of whether to execute the output section or   //
//                       not                                                         //
// output:                                                                           //
// purpose: Prints sub-files in the directory specified by the filename argument     //
//          based on the options                                                     //
///////////////////////////////////////////////////////////////////////////////////////
int wildcard(FILE *file, char* patternName, int isPrint) {
    
    DIR *dirp; //dir 포인터
    struct dirent *dir; //dirent 구조체
    int wildFlag = 0, fileNum = 0, dirNum = 0; //와일드카드의 유무, 파일 개수, 디렉토리 개수
    char *fileList[MAX_LENGTH], *dirList[MAX_LENGTH],  *dirList2[MAX_LENGTH]; //파일 리스트, 디렉토리 리스트, 디렉토리 하위 디렉토리 리스트
    char wildPath[MAX_LENGTH] = {'\0', }, dirPath[MAX_LENGTH] = {'\0', }; //현재 절대 경로, 디렉토리 하위 디렉토리의 절대 경로
    char APatternName[MAX_LENGTH] = {'\0', }; //개선된 패턴

    getAbsolutePath(patternName, wildPath); //현재 디렉토리 절대경로 받기
    getAbsolutePath(patternName, APatternName); //현재 디렉토리 절대경로 받기

    char *lastSlash = strrchr(wildPath, '/'); // 가장 마지막의 '/'를 찾기
    if (lastSlash != NULL)
        *lastSlash = '\0'; // '/'를 null 문자로 바꿔 문자열을 종료
    else
        strcpy(wildPath, getcwd(wildPath, MAX_LENGTH)); //현재 위치 받아오기

    dirp = opendir(wildPath); // 절대경로로 opendir

    while ((dir = readdir(dirp)) != NULL) { //파일 읽어오기 

        char fileName[MAX_LENGTH] = {'\0', }; //파일이름 받아올 배열 선언 및 초기화
        joinPathAndFileName(fileName, wildPath, dir->d_name);

        //와일드카드와 일치하는 디렉토리인 경우
        if((fnmatch(APatternName, fileName, 0) == 0) && (dir->d_name[0] != '.') && (dir->d_type == DT_DIR)) {
            dirList[dirNum] = dir->d_name; //dirList에 추가
            dirNum++; //디렉토리 숫자 증가
            wildFlag = 1; //인자가 와일드카드임을 표시
        }

        // 와일드카드와 일치하는 디렉토리 외 파일인 경우
        else if((fnmatch(APatternName, fileName, 0) == 0) && (dir->d_name[0] != '.')) {
            if (strcmp(dir->d_name, TITLE_HTML) != 0) {
                fileList[fileNum] = dir->d_name; //fileList에 추가
                fileNum++; // 파일 숫자 증가
                wildFlag = 1; // 인자가 와일드카드임을 표시
            } 
        }
    }

    if(wildFlag == 0) //만약 와일드카드가 아니라면
        return wildFlag; //0을 반환    

    if (isPrint == 1) { //출력이 필요한 경우

        sortByNameInAscii(fileList, fileNum, 0, 0); //아스키코드순으로 파일 정렬
        sortByNameInAscii(dirList, dirNum, 0, 0); //아스키코드순으로 디렉토리 정렬

        rewinddir(dirp); //dirp를 초기화
        if(fileNum != 0) 
            fprintf(file, "<table border=\"1\">\n<tr>\n<th>Name</th>\n</tr>\n");

        for (int i = 0; i < fileNum; i++) {//파일리스트만큼 반복하면서 
            
            char filePath[MAX_LENGTH]; //파일의 절대 경로
            joinPathAndFileName(filePath, wildPath, fileList[i]); //절대 경로와 파일이름 붙이기

            char color[20]; //파일의 색
            findColor(filePath, color); //파일 색 찾기

            char printName[MAX_LENGTH] = {'\0', }; //출력할 이름
            strcpy(printName, patternName);
            lastSlash = strchr(printName, '/'); //마지막에 있는 '/' 없애기
            if (lastSlash != NULL)
                *lastSlash = '\0'; // '/'를 null 문자로 바꿔 문자열을 종료
            joinPathAndFileName(printName, printName, fileList[i]); //입력받은 패턴에 따라 출력할 이름 설정

            fprintf(file, "<tr style=\"%s\"><td><a href=%s>%s</a></td></tr>\n", color, filePath, printName); //파일명 출력
        }
        fprintf(file, "</table>\n<br>\n");

        for (int i = 0; i < dirNum; i++) { //디렉토리 개수만큼 반복

            char dirPath[MAX_LENGTH]; //디렉토리 경로 저장 변수 선언 및 초기화
            joinPathAndFileName(dirPath, wildPath, dirList[i]);           

            dirp = opendir(dirPath); //dirPath로 open
            int dirNum2 = 0; //디렉토리 하위 파일들의 개수
            while ((dir = readdir(dirp)) != NULL) { //파일 읽어오기

                if ((dir->d_name[0] != '.') && (strcmp(dir->d_name, TITLE_HTML) != 0)) { //히든파일이 아니라면
                    dirList2[dirNum2] = dir->d_name; //하위 파일명들 저장
                    dirNum2++; //하위 파일들의 개수 세기
                }
            }

            
            if(dirNum2 != 0) { //디렉토리 내 하위 파일이 존재하는 경우

                fprintf(file, "<b>Directory path: %s<b>\n", dirPath); //파일 경로 출력
                sortByNameInAscii(dirList2, dirNum2, 0, 0); // 하위 파일들도 아스키코드 순으로 정렬
                fprintf(file, "<table border=\"1\">\n<tr>\n<th>Name</th>\n</tr>\n"); //테이블 설정
                
                for (int j = 0; j < dirNum2; j++) { // 파일 개수만큼 반복하면서

                    char fileLink[MAX_LENGTH] = {'\0'}; //하이퍼링크로 설정할 경로
                    joinPathAndFileName(fileLink, dirPath, dirList2[j]); //절대경로와 파일 이름 붙이기
                    fprintf(file, "<tr><td><a href=%s>%s</a></td></tr>\n", fileLink, dirList2[j]); // 파일명 출력
                }
                fprintf(file, "</table>\n<br>\n");
            }
        }
    }
    return wildFlag; //와일드카드 여부 반환
}

///////////////////////////////////////////////////////////////////////////////////////
// listDirFiles                                                                      //
// --------------------------------------------------------------------------------- //
// Input: int a_hidden -> option -a                                                  //
//        int l_format -> option -l                                                  //
//        char* filename -> file name that provided                                  //
// output:                                                                           //
// purpose: Prints sub-files in the directory specified by the filename argument     //
//          based on the options                                                     //
///////////////////////////////////////////////////////////////////////////////////////
void listDirFiles(int a_hidden, int l_format, int S_size, int r_reverse, int h_readable, char* filename, FILE *file) {

    DIR *dirp; //dir 포인터
    struct dirent *dir; //dirent 구조체
    struct stat st, fileStat; //파일 속성정보 저장할 구조체
    int fileNum = 0, realfileNum = 0; //파일의 개수
    char timeBuf[80]; //시간 정보
    int total = 0; //총 블락 수
    char accessPath[MAX_LENGTH], accessFilename[MAX_LENGTH], dirPath[MAX_LENGTH] = {'\0', }; //여러 경로들
    int* isHidden = (int*)calloc(fileNum, sizeof(int)); //히든파일 여부 판별

    getAbsolutePath(filename, dirPath); 
    dirp = opendir(dirPath); //절대경로로 opendir

    while((dir = readdir(dirp)) != NULL) { //디렉토리 하위 파일들을 읽어들임

        joinPathAndFileName(accessFilename, dirPath, dir->d_name);
        stat(accessFilename, &st); //파일의 절대 경로로 stat() 호출

        if(a_hidden == 1 || dir->d_name[0] != '.') {
            total += st.st_blocks; //옵션(-a)에 따라 total 계산           
            ++realfileNum; //총 파일개수 count
        }
        ++fileNum; //총 파일개수 count
    }
    
    rewinddir(dirp); //dirp 처음으로 초기화

    char **fileList = (char**)malloc(sizeof(char*) * (fileNum+1)); //동적 할당
    for(int i = 0; i < fileNum; i++) { //파일 개수만큼 반복
        
        fileList[i] = (char*)malloc(sizeof(char) * 300); //동적 할당
        dir = readdir(dirp);
        if((strcmp(fileList[i], TITLE_HTML) != 0)) //html 실행 파일이 아니라면
            strcpy(fileList[i], dir->d_name); //fileList에 파일명 저장
    }

    sortByNameInAscii(fileList, fileNum, 0, r_reverse); //아스키 코드순으로 정렬
    if (S_size == 1) //S 옵션 존재 시
        sortByFileSize(fileList, dirPath, fileNum, 0, r_reverse); //파일 사이즈로 재정렬
    else
        sortByNameInAscii(fileList, fileNum, 0, r_reverse); //fileList를 알파벳 순으로 sort(대소문자 구분 x)

    fprintf(file, "<b>Directory path: %s</b><br>\n", dirPath); //파일 경로 출력
    
    if(l_format == 1) //옵션 -l이 포함된 경우
        fprintf(file, "<b>total : %d</b>\n", (int)(total/2));

    if(realfileNum == 0) { //만약 html 실행 파일과 히든 파일을 제외한 파일이 없다면
        fprintf(file, "<br><br>\n");
        return;
    }

    fprintf(file, "<table border=\"1\">\n<tr>\n<th>Name</th>\n"); // 테이블 생성
    if (l_format == 1) {
        fprintf(file, "<th>Permissions</th>\n");        // 권한 열 생성
        fprintf(file, "<th>Link</th>\n");               // 링크 열 생성
        fprintf(file, "<th>Owner</th>\n");              // 소유자 열 생성
        fprintf(file, "<th>Group</th>\n");              // 소유 그룹 열 생성
        fprintf(file, "<th>Size</th>\n");               // 사이즈 열 생성
        fprintf(file, "<th>Last Modified</th>\n</tr>"); // 마지막 수정날짜 열 생성
    }
    else
        fprintf(file, "</tr>");

    for (int i = 0; i < fileNum; i++) { // 파일 개수만큼 반복

        if ((a_hidden == 0) && fileList[i][0] == '.') // 옵션 -a 여부에 따라 파일속성 출력
            continue;

        joinPathAndFileName(accessPath, dirPath, fileList[i]); // 파일과 경로를 이어붙이기
        stat(accessPath, &fileStat);                           // 파일 속성정보 불러옴

        char color[20] = {'\0',};                            // 파일의 색상 저장할 배열
        findColor(accessPath, color); // 색 찾기

        fprintf(file, "<tr style=\"%s\">\n", color);
        fprintf(file, "<td><a href=%s>%s</a></td>", accessPath, fileList[i]); // 파일 이름 및 링크 출력

        if (l_format == 1)                                      // 옵션 -l이 포함된 경우
            printAttributes(fileStat, file, h_readable, color); // 속성 정보 출력

        else
            fprintf(file, "</tr>");
    }
    fprintf(file, "</table>\n<br>\n");
}

///////////////////////////////////////////////////////////////////////////////////////
// printFileInfo                                                                     //
// --------------------------------------------------------------------------------- //
// Input: char* l_format -> option -l                                                //
//        int filename -> file name that provided                                    //
// output:                                                                           //
// purpose: Prints the attributes of the file based on the presence or absence of    //
//          the -l option, given the filename argument.                              //
///////////////////////////////////////////////////////////////////////////////////////
void printFileInfo(char* fileName, FILE *file, int l_format, int h_readable) {
    
    struct stat fileStat; //파일 속성정보 받아올 변수
    char timeBuf[80]; //파일 시간정보 받아올 변수
    char filePath[MAX_LENGTH]; //파일 경로 저장할 변수

    fprintf(file, "<table border=\"1\">\n<tr>\n<th>Name</th>\n"); // 테이블 생성
    if (l_format == 1) { //-l 옵션이 존재할 경우
        fprintf(file, "<th>Permissions</th>\n");        // 권한 열 생성
        fprintf(file, "<th>Link</th>\n");               // 링크 열 생성
        fprintf(file, "<th>Owner</th>\n");              // 소유자 열 생성
        fprintf(file, "<th>Group</th>\n");              // 소유 그룹 열 생성
        fprintf(file, "<th>Size</th>\n");               // 사이즈 열 생성
        fprintf(file, "<th>Last Modified</th>\n</tr>"); // 마지막 수정날짜 열 생성
    }
    else
        fprintf(file, "</tr>"); // 그냥 한 행 닫기

    getAbsolutePath(fileName, filePath); //절대경로 찾기
    stat(filePath, &fileStat); //절대경로로 파일 속성 받기

    char color[20] = {'\0', }; //파일의 유형에 따른 색 저장할 배열
    findColor(filePath, color); //색을 찾을 함수 호출

    fprintf(file, "<tr style=\"%s\">\n", color); //파일 색상 결정
    fprintf(file, "<td><a href=%s>%s</a></td>", filePath, fileName); //파일 이름 및 링크 출력

    if(l_format == 1) //옵션 -l이 포함된 경우 파일 속성 출력
        printAttributes(fileStat, file, h_readable, color);
    else
        fprintf(file, "</tr>\n");
}

///////////////////////////////////////////////////////////////////////////////////////
// getAbsolutePath                                                                   //
// --------------------------------------------------------------------------------- //
// Input: char* inputPath -> Path input                                              //
//        char* absolutePath -> path that made asolute                               //
// output:                                                                           //
// purpose: Finds the absolute path of the inputted directory or file.               //
///////////////////////////////////////////////////////////////////////////////////////
void getAbsolutePath(char *inputPath, char *absolutePath) {

    getcwd(absolutePath, MAX_LENGTH); //현재 경로 받아옴

    if(inputPath[0] != '/') //입력이 절대경로가 아닌 파일이고, /로 시작하지 않을 때 ex) A/*
        strcat(absolutePath, "/"); // /을 제일 앞에 붙여줌
    
    if(strstr(inputPath, absolutePath) != NULL) //입력받은 경로가 현재 경로를 포함할 때 ex)/home/Assignment/A/*
        strcpy(absolutePath, inputPath); //입력받은 경로로 절대경로 덮어쓰기
    else
        strcat(absolutePath, inputPath); //현재 경로에 입력받은 경로 이어붙이기
}

///////////////////////////////////////////////////////////////////////////////////////
// joinPathAndFileName                                                               //
// --------------------------------------------------------------------------------- //
// Input: char* inputPath -> new array that concatenated                             //
//        char* Apath -> absolute path as input                                      //
//        char* fileName -> file name that appended                                  //
// output:                                                                           //
// purpose: Receives an absolute path as input, along with a file name to be         //
//          appended, and generates a new array representing the concatenated path.  //
///////////////////////////////////////////////////////////////////////////////////////
void joinPathAndFileName(char* path, char* Apath, char* fileName) {

    strcpy(path, Apath); //입력받은 경로 불러오기
    strcat(path, "/"); // /를 붙이고
    strcat(path, fileName); //읽어온 파일명 붙이기
}

///////////////////////////////////////////////////////////////////////////////////////
// printAttributes                                                                   //
// --------------------------------------------------------------------------------- //
// Input: struct stat fileStat -> Save information about a file (such as file size   //
//                                , owner, permissions, etc.)                        //
// output:                                                                           //
// purpose: Prints the attributes of the file using the information from the given   //
//          name of struct stat object                                               //
///////////////////////////////////////////////////////////////////////////////////////
void printAttributes(struct stat fileStat, FILE* file, int h_readable, char *color) {
    
    char timeBuf[80]; //시간 정보 받아올 변수

    printType(fileStat, file); // 파일 유형
    printPermissions(fileStat.st_mode, file); // 허가권
    fprintf(file, "<td>%ld</td>", fileStat.st_nlink); // 링크 수
    fprintf(file, "<td>%s</td><td>%s</td>", getpwuid(fileStat.st_uid)->pw_name, getgrgid(fileStat.st_gid)->gr_name); // 파일 소유자 및 파일 소유 그룹

    if(h_readable == 1) { //만약 h 속성이 존재한다면
        
        double size = (double)fileStat.st_size; //파일의 사이즈를 받아오기
        int remainder = 0; //1024로 나눈 나머지를 계속 저장할 변수
        char sizeUnit[3] = {'K', 'M', 'G'}; //K, M, G 단위
        int unit = 0, unitIndex = -1; //유닛 인덱스로 단위 붙이기

        while (size >= 1024) { //1024보다 클 경우
            size /= 1024; //1024로 나눈 몫
            remainder %= 1024; //1024로 나눈 나머지
            unit = 1; //단위를 붙여야 함
            unitIndex++; //유닛 인덱스 증가
        }

        if(unit == 1) //단위를 붙여야 한다면
            fprintf(file, "<td>%.1f%c</td>", size, sizeUnit[unitIndex]); //소수점 아래 한 자리까지 출력
        else //안붙여도 될 정도로 작다면
            fprintf(file, "<td>%.0f</td>", size); //소수점 버리고 출력
    }
    else
        fprintf(file, "<td>%ld</td>", fileStat.st_size); // 파일 사이즈

    strftime(timeBuf, sizeof(timeBuf), "%b %d %H:%M", localtime(&fileStat.st_mtime)); // 수정된 날짜 및 시간 불러오기
    fprintf(file, "<td>%s</td>", timeBuf); // 수정된 날짜 및 시간 출력

    fprintf(file, "</tr>\n");
}

///////////////////////////////////////////////////////////////////////////////////////
// sortByNameInAscii                                                                 //
// --------------------------------------------------------------------------------- //
// Input: char **fileList -> An array containing file names.                         //
//        int fileNum -> size of fileList                                            //
// output:                                                                           //
// purpose: Sort the filenames in alphabetical order (ignoring case) without the dot //
///////////////////////////////////////////////////////////////////////////////////////
void sortByNameInAscii(char **fileList, int fileNum, int start, int r_reverse)
{
    int* isHidden = (int*)calloc(fileNum, sizeof(int)); //hidden file인지 판별 후 저장
    
    for (int i = start; i < fileNum; i++) { //파일리스트 반복문 실행
         if ((fileList[i][0] == '.') && (strcmp(fileList[i], ".") != 0) && (strcmp(fileList[i], "..") != 0)) { //hidden file인 경우
            isHidden[i] = 1; //파일명 가장 앞의 . 제거
            for (int k = 0; k < strlen(fileList[i]); k++) //파일 글자수 반복
                fileList[i][k] = fileList[i][k + 1]; //앞으로 한 칸씩 땡기기
        }
    }

    for (int i = start; i < (fileNum - 1); i++) { // 대소문자 구분 없는 알파벳 순으로 정렬
        for (int j = i + 1; j < fileNum; j++) { //bubble sort
            if (((compareStringUpper(fileList[i], fileList[j]) == 1) && (r_reverse == 0)) || ((compareStringUpper(fileList[i], fileList[j]) == 0) && (r_reverse == 1))) {
            //만약 첫 문자열이 둘째 문자열보다 작다면
                char *temp = fileList[i]; // 문자열 위치 바꾸기
                fileList[i] = fileList[j];
                fileList[j] = temp;

                int temp2 = isHidden[i]; //히든파일인지 저장한 배열도 위치 바꾸기
                isHidden[i] = isHidden[j];
                isHidden[j] = temp2;
            }
        }
    }

    for (int i = start; i < fileNum; i++) { //리스트 반복문 돌리기
        if(isHidden[i] == 1) { //hidden file인 경우
            for(int k = strlen(fileList[i]); k >= 0; k--) //파일 길이만큼 반복
                fileList[i][k+1] = fileList[i][k]; //뒤로 한 칸씩 보내기
            fileList[i][0] = '.'; //파일명 가장 앞에 . 다시 추가
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////
// sortByFileSize                                                                    //
// --------------------------------------------------------------------------------- //
// Input: char **fileList -> An array containing file names                          //
//        char *dirPath -> path of file                                              //
//        int fileNum -> size of file                                                //
//        int start -> where to start to sort files                                  //
//        int r_reverse -> reverse sort flag                                         //
// output:                                                                           //
// purpose: Comparing two strings based on uppercase letters to determine which one  //
//          is greater.                                                              //
///////////////////////////////////////////////////////////////////////////////////////
void sortByFileSize(char **fileList, char *dirPath, int fileNum, int start, int r_reverse) {
    
    struct stat fileStat1, fileStat2; //stat of files

    for (int i = start; i < (fileNum - 1); i++) { //bubble sort
        for (int j = i + 1; j < fileNum; j++) { 
            
            char accessFilename1[MAX_LENGTH]; //first file name
            joinPathAndFileName(accessFilename1, dirPath, fileList[i]);
            stat(accessFilename1, &fileStat1); //filestat open

            char accessFilename2[MAX_LENGTH]; //second file name
            joinPathAndFileName(accessFilename2, dirPath, fileList[j]);
            stat(accessFilename2, &fileStat2); //filestat open

            if (((fileStat1.st_size < fileStat2.st_size) && (r_reverse == 0)) || ((fileStat1.st_size > fileStat2.st_size) && (r_reverse == 1))) { //뒤의 문자열이 더 크다면
                
                char *temp = fileList[j]; // 문자열 위치 바꾸기
                for (int k = j; k > i; k--)
                    fileList[k] = fileList[k - 1]; //앞으로 한 칸씩 당기고
                fileList[i] = temp; //첫 번째 문자열이 제일 뒤 칸으로 가기
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////
// compareStringUpper                                                                //
// --------------------------------------------------------------------------------- //
// Input: char* fileName1 -> The first string to compare                             //
//        char* fileName2 -> The second string to compare                            //
// output: 0 -> No need to swap positions, 1 -> Need to swap positions               //
// purpose: Comparing two strings based on uppercase letters to determine which one  //
//          is greater.                                                              //
///////////////////////////////////////////////////////////////////////////////////////
int compareStringUpper(char* fileName1, char* fileName2) {
    
    char* str1 = (char*)calloc(strlen(fileName1)+1, sizeof(char)); //비교할 첫 번째 문자열
    char* str2 = (char*)calloc(strlen(fileName2)+1, sizeof(char)); //비교할 두 번째 문자열

    for(int i = 0; i < strlen(fileName1); i++) //첫 번째 문자열을 돌면서
        str1[i] = toupper(fileName1[i]); //모두 대문자로 전환

    for(int i = 0; i < strlen(fileName2); i++) //두 번째 문자열을 돌면서
        str2[i] = toupper(fileName2[i]); //모두 대문자로 전환

    if((strcmp(str1, ".") == 0 || strcmp(str1, "..") == 0) && (strcmp(str2, ".") != 0)) //위치를 바꿀 필요가 없는 경우
        return 0; //0 반환

    else if((strcmp(str2, ".") == 0 || strcmp(str2, "..") == 0) && (strcmp(str1, ".") != 0)) //위치를 바꿀 필요가 있는 경우
        return 1; //1 반환

    else if(strcmp(str1, str2) > 0) //위치를 바꿀 필요가 있는 경우
        return 1; //1 반환
    
    return 0; //위치를 바꿀 필요가 없는 경우 0 반환
}

///////////////////////////////////////////////////////////////////////////////////////
// printPermissions                                                                  //
// --------------------------------------------------------------------------------- //
// Input: mode_t mode -> represents the permission information of a file.            //
// output:                                                                           //
// purpose: Printing file permissions for user, group, and others.                   //
///////////////////////////////////////////////////////////////////////////////////////
void printPermissions(mode_t mode, FILE *file) {
    fprintf(file, "%c", (mode & S_IRUSR) ? 'r' : '-'); //user-read
    fprintf(file, "%c", (mode & S_IWUSR) ? 'w' : '-'); //user-write
    fprintf(file, "%c", (mode & S_IXUSR) ? 'x' : '-'); //user-execute
    fprintf(file, "%c", (mode & S_IRGRP) ? 'r' : '-'); //group-read
    fprintf(file, "%c", (mode & S_IWGRP) ? 'w' : '-'); //group-write
    fprintf(file, "%c", (mode & S_IXGRP) ? 'x' : '-'); //group-execute
    fprintf(file, "%c", (mode & S_IROTH) ? 'r' : '-'); //other-read
    fprintf(file, "%c", (mode & S_IWOTH) ? 'w' : '-'); //other-write
    fprintf(file, "%c", (mode & S_IXOTH) ? 'x' : '-'); //other-execute

    fprintf(file, "</td>");
}

///////////////////////////////////////////////////////////////////////////////////////
// printType                                                                         //
// --------------------------------------------------------------------------------- //
// Input: struct stat fileStat -> Save information about a file (such as file size   //
//                                , owner, permissions, etc.)                        //
// output:                                                                           //
// purpose: Printing file type(regular file, directory, symbolic link, etc.)         //
///////////////////////////////////////////////////////////////////////////////////////
void printType(struct stat fileStat, FILE *file) {

    fprintf(file, "<td>");

    switch (fileStat.st_mode & __S_IFMT) {
    case __S_IFREG: //regular file
        fprintf(file, "-");
        break;
    case __S_IFDIR: //directory
        fprintf(file, "d");
        break;
    case __S_IFLNK: //symbolic link
        fprintf(file, "l");
        break;
    case __S_IFSOCK: //socket
        fprintf(file, "s");
        break;
    case __S_IFIFO: //FIFO(named pipe)
        fprintf(file, "p");
        break;
    case __S_IFCHR: //character device
        fprintf(file, "c");
        break;
    case __S_IFBLK: //block device
        fprintf(file, "b");
        break;
    default:
        fprintf(file, "?"); //unknown
        break;
    }
}

///////////////////////////////////////////////////////////////////////////////////////
// findColor                                                                         //
// --------------------------------------------------------------------------------- //
// Input: char* fileName -> File name to determine the file type                     //
//        char* color -> Array to store colors based on the file type                //
// output:                                                                           //
// purpose: Detects the file type based on the inputted file name and stores the     //
//          corresponding color in an array.                                         //
///////////////////////////////////////////////////////////////////////////////////////
void findColor(char* fileName, char* color) {

    struct stat fileStat; //파일 속성정보
    stat(fileName, &fileStat); //파일 경로로 속성 정보 받아오기
    
    if ((fileStat.st_mode & __S_IFMT) == __S_IFDIR) //디렉토리일 경우
        strcpy(color, "color: Blue"); //파랑 출력
    else if ((fileStat.st_mode & __S_IFMT) == __S_IFLNK) //심볼릭 링크일 경우
        strcpy(color, "color: Green"); //초록 출력
    else //그 외 파일들의 경우
        strcpy(color, "color: Red"); //빨강 출력
}