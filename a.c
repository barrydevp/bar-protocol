#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint16_t a;
    char* b;
} test;

int main() {
    char** num = malloc(4 * sizeof(char*));
    for (int i = 0; i < 4; i++) {
        printf("%p\n", num + i);
        /* *(num + i) = i + 1; */
        /* printf("%d\n", num[i]); */
    }

    printf("sizeof(char*) %d\n", sizeof(char*));
    printf("sizeof(char**) %d\n", sizeof(char**));
    printf("sizeof(int*) %d\n", sizeof(int*));
    printf("sizeof(int) %d\n", sizeof(int));
    printf("sizeof(char) %d\n", sizeof(char));
    printf("sizeof(num) %d\n", sizeof(num));
    char num2[10];
    char* num3 = num2;
    printf("sizeof(num2) %d\n", sizeof(num2));
    printf("sizeof(num3) %d\n", sizeof(num3));

    uint8_t i = 255;
    uint8_t j = 255;
    printf("%d\n", i * j);

    test _test = {};
    char buffer[1024]; 
    char* non = NULL;

    memcpy(buffer, &_test, sizeof(_test));
    printf("%s\n", buffer);

    _test.a = 1;
    _test.b = "HELLO";
    memcpy(buffer, &_test, sizeof(_test));
    printf("%s\n", buffer);

    printf("----\n%s\n----\n", non);

    return 0;
}
