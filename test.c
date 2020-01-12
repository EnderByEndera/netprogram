#include <stdio.h>

int main(int argc, char *argv[])
{
    char password[8] = "secret";
    char input[8];
    while(1)
    {
        printf("Enter the password:");
        fgets(input, 8, stdin);
        if (strcmp(input, password) == 0)
        {
            printf("Welcome!\n");
            break;
        }
        else 
        {
            printf("Password incorrect, please try again,\n");
        }
    }
}