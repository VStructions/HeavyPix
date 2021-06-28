/*
------------------------------------------------------------------------------
MIT License
Copyright (c) 2021 VStructions
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
*/
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_lib/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_lib/stb_image_write.h"

#define STBI_ONLY_PNG
#define STBI_ONLY_BMP
#define STBI_ONLY_GIF
#define STBI_ONLY_JPEG  //Reads these files but only writes BMP

#define LSB_0 0xFE
#define LSB_1 0x01

void getAction(int argc, char** args, int* action, char** file);
void options(int action, char** file);
int readMessage(char* filename, char* txtfilename);
int writeMessage(char* filename, char* txtfilename, char* readyMsg);
int getMsg(char* line, int MAXLEN);
int fileToString(char* msg, FILE* cursor, int maxLen);

int main (int argc, char** args) {

    int action = -1;
    char** file = (char**) malloc(3 * sizeof(char*));       //file[0] filename, file[1] txt filename, file[2] consoleMsg

    getAction(argc, args, &action, file);
    if(action == -2)
        goto cleanUp;

    options(action, file);
    
    cleanUp:
    free(file);

    return 0;
}

void getAction(int argc, char** args, int* action, char** file)
{
    int i=1;
	
    if(argc > 1 && argc < 5) 
    {
        if(args[i][0] == '-')
        {
            if(strlen(args[i]) == 2)
            {    
                if(args[i][1] == 'r')
                {
                    *action = 0;
                }
                else if(args[i][1] == 'w') 
                {
                    *action = 1;
                }
                else if(args[i][1] == 'c')
                {
                    *action = 2;
                    if(argc-1 >= ++i)
                        file[1] = args[i];
                    else
                        goto few;
                }
                else if(args[i][1] == 's')
                {
                    *action = 3;
                    if(argc-1 >= ++i)
                        file[2] = args[i];
                    else
                        goto few;

                }
                else if(args[i][1] == 'p')
                {
                    *action = 4;
                    if(argc-1 >= ++i)
                        file[1] = args[i];
                    else
                        goto few;
                }
            }
            if(argc-1 >= ++i)
                file[0] = args[i];
                else
                    goto few;
        }
        else
        {
            file[0] = args[i];
            *action = 0; 
        }       
    }
    else if(argc > 4)
    {
        printf("Error: Too many parameters.");
        *action = -2;
        return;
    }
    else
    {
        printf("Author: VStructions\nProgram: HeavyPix\nUsage: reads or writes hidden text in bmp, png and GIF images.\nValid parameters:\n\n[-r \"imageFilename\"] (read),                              [\"imageFilename\"] (read), \n");
        printf("[-w \"imageFilename\"] (write),                             [-s \"String here\" \"imageFilename\"] (write from console),\n");
        printf("[-c \"textFilename\" \"imageFilename\"] (copy from txt file), [-p \"textFilename\" \"imageFilename\"] (paste to txt file).");
        getchar();
        *action = -2;
        return ;
    }
    if(0)
    {
        few:
        printf("Error: Too few arguements.");
        *action = -2;
        return ;
    }
}

void options(int action, char** file)
{
    if(action == 1)
    {
        writeMessage(file[0], NULL, NULL);
    }
    else if(action == 2)
    {
        writeMessage(file[0], file[1], NULL);
    }
    else if(action == 3)
    {
        writeMessage(file[0], NULL, file[2]);
    }
    else if(action == 4)
    {
        readMessage(file[0], file[1]);
    }   
    else if(!action)
    {
        readMessage(file[0], NULL);
        getchar();
    }
}

int readMessage(char* filename, char* txtfilename) 
{
    int index = 0, bitCounter = 0, i, height, width, allChannelsPerPixels, components, charCount;
    unsigned char *msg;
    uint8_t  lsbHandle;
    FILE* cursor;

    uint8_t* channel = stbi_load( filename, &width, &height, &components, 0);
    if( channel == NULL )
    {
        printf("Error: Image loading failed.");
        return -1;
    }
    allChannelsPerPixels = width * height * components;
    charCount = allChannelsPerPixels/8;

    msg = (unsigned char*) malloc( charCount * sizeof(unsigned char));
    if( msg == NULL) 
    {
        printf("Error: Memory allocation for the message failed.");
        free(channel);
        return -2;
    }
    for( i=0; i < charCount; i++) 
        msg[i] = 0;
    
    for( i=0; i < allChannelsPerPixels; i++) 
    {
        lsbHandle = channel[i];
        lsbHandle &= LSB_1; //Preserve only LSB

        msg[index] |= lsbHandle;

        bitCounter++;
        if(bitCounter == 8) 
        {
            bitCounter = 0;
            index++;
        }
        else 
        {
            msg[index] <<= 1;
        }        
    }

    if(txtfilename)
    {
        if( !(cursor = fopen( txtfilename, "a")) )
        {
            printf("Error: Text file creation failed.");
            free(msg);
	        free(channel);
            return -1;
        }
        fprintf(cursor, "%s", msg);
        fclose(cursor);
    }
    else
        printf("%s", msg);

    free(msg);
	free(channel);

    return 0;
}

int writeMessage(char* filename, char* txtfilename, char* readyMsg)
{
    int index = 0, bitCounter = 7, i, height, width, allChannelsPerPixels, components, charCount, msgLen;
    unsigned char newFilename[103] = {'N', 'e', 'w'};
    unsigned char *msg;
    FILE* cursor;
    uint8_t  charHandle;

    if(txtfilename)
        if( !(cursor = fopen( txtfilename, "r")) )
        {
            printf("Error: Text file loading failed.");
            return -1;
        }

    uint8_t* channel = stbi_load( filename, &width, &height, &components, 0);
    if( channel == NULL )
    {
        printf("Error: Image loading failed.");
        return -2;
    }
    allChannelsPerPixels = width * height * components;
    charCount = (allChannelsPerPixels/8) - 3; //-3 just in case

    msg = (unsigned char*) malloc( charCount * sizeof(unsigned char));
    if( msg == NULL) 
    {
        printf("Error: Memory allocation for the message failed.");
        free(channel);
        return -3;
    }
    for( i=0; i < charCount; i++) 
        msg[i] = '\0';
    
    if(readyMsg)
    {
        strncat(msg, readyMsg, charCount-1);
    }
    else if(txtfilename)
    {
        fileToString(msg, cursor, charCount);
        fclose(cursor);
    }
    else
    {
        printf("Enter message (%d characters, end with \"\\end\"): ", charCount);
        getMsg(msg, charCount);
    }

    for( i=0; i < allChannelsPerPixels; i++) 
    {
        charHandle = msg[index];
        charHandle >>= bitCounter;
        charHandle &= LSB_1;

        channel[i] &= LSB_0;
        channel[i] |= charHandle;

        if(bitCounter == 0)
        {
            bitCounter = 7;
            index++;
        }
        else
            bitCounter--;
    }

    strcat(newFilename, filename);
    stbi_write_bmp(newFilename, width, height, components, channel);

    free(msg);
	free(channel);

    return 0;
}

int getMsg(char* line, int MAXLEN)  //Getline
{
    int d, i, c, e, n;

    for(i=0; i < MAXLEN-1 && (c=getchar()) != EOF; i++)
    {
        if( c == '\\' )
        {
            if( (e=getchar()) == 'e' )
            {
                if( (n=getchar()) == 'n' )
                {
                    if( (d=getchar()) == 'd' )
                        break;
                    else
                        ungetc(d, stdin);
                }
                ungetc(n, stdin);
            }
            ungetc(e, stdin);
        }
        line[i] = c;
    }

    line[i] = '\0';
    return i;
}

int fileToString(char* msg, FILE* cursor, int maxLen)
{
    int index = 0;
    char buffer[maxLen];
    
    while( fgets(buffer, maxLen, cursor) != NULL )
        if(strlen(buffer) < maxLen - index)
        {
            strcat(msg+index, buffer);
            index = strlen(msg);
        }
        else
        {
            printf("Error: Text file too big for this image, copied only a fitting portion.");
            getchar();
            return -1;
        }
        
    return 0;
}