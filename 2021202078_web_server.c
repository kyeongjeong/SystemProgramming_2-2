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

#define _GNU_SOURCE
#define FNM_CASEFOLD  0x10
#define MAX_LENGTH 1000
#define DIR_FILE_LIST "html_ls.html"
#define URL_LEN 256
#define BUFSIZE 1024
#define PORTNO 40000


void sendResponse(char* url, char* response_header, int isNotStart, int client_fd);
void listDirFiles(int a_hidden, int l_format, int S_size, int r_reverse, int h_readable, char* filename, FILE *file);
void getAbsolutePath(char *inputPath, char *absolutePath);
void joinPathAndFileName(char* path, char* Apath, char* fileName);
void sortByNameInAscii(char **fileList, int fileNum, int start, int r_reverse);
void printPermissions(mode_t mode, FILE *file);
void printType(struct stat fileStat, FILE *file);
void findColor(char* fileName, char* color);
void printAttributes(struct stat fileStat, FILE *file, int h_readable, char *color);
int compareStringUpper(char* fileName1, char* fileName2);
int writeHTMLFile(char* url);

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
int main() {
    
    struct sockaddr_in server_addr, client_addr;
    int socket_fd, client_fd;
    int len_out;
    unsigned int len;
    int opt = 1;
    int count = 0;
    int isNotStart = 0;
    char ch;
    char Apath[MAX_LENGTH] = {'\0', }; //working directory의 절대 경로
    char title[MAX_LENGTH] = "html_ls.html"; // html 문서

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
        FILE *file;

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

        if(strcmp(url, "/favicon.ico") == 0)
            continue;
        sendResponse(url, response_header, isNotStart, client_fd);

        isNotStart = 1;
        printf("[%s : %d] client was disconnected\n", inet_ntoa(inet_clinet_address), client_addr.sin_port);
        close(client_fd);       
    }
    close(socket_fd);
    return 0;
}

void sendResponse(char* url, char* response_header, int isNotStart, int client_fd) {

    FILE *file;
    DIR *dirp; //DIR 포인터
    struct dirent *dir; //dirent 구조체
    int count = 0, isNotFound = 0;
    char ch;
    char file_extension[10]; // 파일 확장자를 저장할 배열
    char content_type[30];   // MIME TYPE을 저장할 배열

    if(isNotStart == 0)
        isNotFound = writeHTMLFile(url);
    
    if (isNotFound == 1) {

        char response_message[MAX_LENGTH];
        sprintf(response_message, "<h1>Not Found</h1><br>"
                                  "The request URL %s was not found on this server<br>"
                                  "HTTP 404 - Not Page Found", url);
        sprintf(response_header, "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n");

        write(client_fd, response_header, strlen(response_header));
        write(client_fd, response_message, strlen(response_message));
        return;
    }

    if ((fnmatch("*.jpg", url, FNM_CASEFOLD) == 0) || (fnmatch("*.png", url, FNM_CASEFOLD) == 0) || (fnmatch("*.jpeg", url, FNM_CASEFOLD) == 0)) 
        strcpy(content_type, "image/*");

    else {
        char *dot = strrchr(url, '.');
        if (dot && dot != url)
            strcpy(file_extension, dot + 1);

        char dirPath[MAX_LENGTH] = {'\0', };
        getAbsolutePath(url, dirPath);

        if((opendir(dirPath) != NULL) || (isNotStart == 0)) {
            strcpy(content_type, "text/html");  
            writeHTMLFile(url);  
        }
        else
            strcpy(content_type, "text/plain");
    }

    if (strcmp(content_type, "text/html") == 0)
        file = fopen(DIR_FILE_LIST, "r");
    else if(strcmp(content_type, "text/plain") == 0)
        file = fopen(url, "r");
    else
        file = fopen(url, "rb");

    while ((ch = fgetc(file)) != EOF)
        count++;

    char *response_message = (char *)malloc(count+1);

    printf("%s: %d -> %ld\n", url, count, strlen(response_message));
    rewind(file);
    fread(response_message, sizeof(char), count, file);
    fclose(file);

    sprintf(response_header, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", content_type);
    write(client_fd, response_header, strlen(response_header));
    write(client_fd, response_message, strlen(response_message));
}

int writeHTMLFile(char* url) {
    
    DIR *dirp; //DIR 포인터
    struct dirent *dir; //dirent 구조체
    char Apath[MAX_LENGTH] = {'\0', }; //working directory의 절대 경로
    char title[MAX_LENGTH] = "html_ls.html"; // html 문서
    
    FILE *file = fopen(DIR_FILE_LIST, "w"); //html 파일 생성 및 오픈

    fprintf(file, "<!DOCTYPE html>\n<html>\n<head>\n"); //title 생성
    getAbsolutePath(title, Apath); //html 문서의 절대 경로 받아오기
    fprintf(file, "<title>%s</title>\n</head>\n<body>\n", Apath); //절대경로로 title 설정

    if(url[1] == '\0') {

        fprintf(file, "<h1>Welcome to System Programming Http</h1>\n<br>\n"); //header 작성
        char currentPath[10] = "."; //현재 경로
        listDirFiles(0, 1, 0, 0, 0, currentPath, file); //현재 디렉토리 하위 파일 출력
    }

    else {       
        char dirPath[MAX_LENGTH] = {'\0', };

        fprintf(file, "<h1>System Programming Http</h1>\n<br>\n"); //header 작성

        getAbsolutePath(url, dirPath);
        if(opendir(dirPath) == NULL)
            return 1;

        listDirFiles(0, 1, 0, 0, 0, url, file);
    }

    fclose(file);
    return 0;
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
        if((strcmp(fileList[i], DIR_FILE_LIST) != 0)) //html 실행 파일이 아니라면
            strcpy(fileList[i], dir->d_name); //fileList에 파일명 저장
    }

    sortByNameInAscii(fileList, fileNum, 0, r_reverse); //아스키 코드순으로 정렬
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