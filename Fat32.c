
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdint.h>
#include <ctype.h>
#define _GNU_SOURCE
#define WHITESPACE " \t\n"
#define MAX_COMMAND_SIZE 255    // The maximum command-line size
#define MAX_NUM_ARGUMENTS 32

struct __attribute__((__packed__)) DirectoryEntry {
    char DIR_Name[11];
    uint8_t DIR_Attr;
    uint8_t DIR_Unused[8];
    uint16_t DIR_FirstClusterHigh;
    uint8_t Unused[4];
    uint16_t DIR_FirstClusterLow;
    uint32_t DIR_FileSize;
};
struct DirectoryEntry dir[16];

char BS_OEMName[8];
int16_t BPB_BytsPerSec;
int8_t BPB_SecPerClus;
int16_t BPB_RsvdSecCnt;
int8_t BPB_NumFATs;
int16_t BPB_RootEntCnt;
char BS_VolLab[11];
int32_t BPB_FatsZ32;
int32_t BPB_RootClus;
int8_t BPB_ExtFlags;
int8_t BPB_FSInfo;

int32_t RootDirSectors = 0;
int32_t FirstDataSector = 0;
int32_t FirstSectorofCluster = 0;

//FILE *fp = NULL; // this will just help with the function declerations throwing errors

// parameters: current sector number that points to a block of data
// returns: the value of the address for that block of data
// desc: finds the starting address of a block of data given the sector number
//    corresponding to that data block
FILE *fp = NULL;
FILE *ofd = NULL;

void printDirectoryEntryAttributes(struct DirectoryEntry *entry) 
{
  printf("File Name: %.11s\n", entry->DIR_Name);
  printf("Attributes: 0x%X\n", entry->DIR_Attr);
  printf("First Cluster High: %d\n", entry->DIR_FirstClusterHigh);
  printf("First Cluster Low: %d\n", entry->DIR_FirstClusterLow);
  printf("File Size: %d bytes\n", entry->DIR_FileSize);
}


int LBAToOffset (int32_t sector) 
{
  return (( sector - 2) * BPB_BytsPerSec) + (BPB_BytsPerSec * BPB_RsvdSecCnt) + (BPB_NumFATs * BPB_FatsZ32 * BPB_BytsPerSec);
}

//purpose: given a logical block address, look up into the first FAT and return the logical
// block address of the block in the file. If there is no further blocks then return -1

int16_t NextLB (uint32_t sector)
{
  uint32_t FATAddress = (BPB_BytsPerSec * BPB_RsvdSecCnt ) + (sector * 4);
  int16_t val;
  fseek (fp, FATAddress, SEEK_SET);
  fread (&val, 2, 1, fp);
  return val;
}

void convertTo8Dot3Format(const char *input, char *output)
{
  memset(output, ' ', 11);

  char inputCopy[13]; 
  strncpy(inputCopy, input, 12);
  inputCopy[12] = '\0'; 

  char *token = strtok(inputCopy, ".");
  if (token)
  {
    strncpy(output, token, (strlen(token) > 8) ? 8 : strlen(token));
  }

  token = strtok(NULL, ".");
  if (token)
  {
    strncpy(output + 8, token, (strlen(token) > 3) ? 3 : strlen(token));
  }
  output[11] = '\0';

  for (int i = 0; i < 11; i++)
  {
    output[i] = toupper(output[i]);
  }
}

void print_Error()
{
  char error_message[30] = "An error has occurred\n";
  write(STDERR_FILENO, error_message, strlen(error_message)); 
}

int main( int argc, char * argv[] )
{

  //FILE *fp = fopen("fat32.img", "r");
  char * command_string = (char*) malloc( MAX_COMMAND_SIZE );

  while( 1 )
  {
    // Print out the msh prompt
    printf ("msh> ");

    // Read the command from the commandi line.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something.
    while( !fgets (command_string, MAX_COMMAND_SIZE, stdin) );

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    int token_count = 0;                                 

    char *argument_pointer;                                         
                                                           
    char *working_string  = strdup( command_string );                
    
    char *head_ptr = working_string;
    
    // Tokenize the input with whitespace used as the delimiter
    while ( ( (argument_pointer = strsep(&working_string, WHITESPACE ) ) != NULL) &&(token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( argument_pointer, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }


    if( token[0] == NULL)
    {
      //free( head_ptr);
      continue;
    }

    if( strcmp(token[0], "open") == 0 && token[1] == NULL)
    {
      printf("USAGE ERROR...open <filename>\n");
      if(fp != NULL)
      {
        fclose(fp);
      }
      continue;
    }

    //handles invalid file erroer as well as openeing a new file 
    if( ( strcmp(token[0], "open") == 0) && token[1] != NULL)
    {
      if( fp != NULL)
      {
        printf("A FILE IS ALREADY OPEN\n");
        continue;
      }

      printf("OPEN\n");
      fp = fopen( token[1], "r");
      fseek(fp, 11, SEEK_SET); //BytsPerSec
      fread( &BPB_BytsPerSec, 2, 1, fp);

      fseek(fp, 36, SEEK_SET); //FatsZ32
      fread( &BPB_FatsZ32, 4, 1, fp);

      fseek(fp, 13, SEEK_SET); //SecPerClus
      fread( &BPB_SecPerClus, 2, 1, fp);

      fseek(fp, 14, SEEK_SET); //RsvdSecCnt
      fread( &BPB_RsvdSecCnt, 2, 1, fp);

      fseek(fp, 16, SEEK_SET); //NumFATS
      fread( &BPB_NumFATs, 1, 1, fp);

      fseek(fp, 40, SEEK_SET); //ExtFlags
      fread( &BPB_ExtFlags, 2, 1, fp);

      fseek(fp, 44, SEEK_SET); //RootClus
      fread( &BPB_RootClus, 4, 1, fp);

      fseek(fp, 48, SEEK_SET); // FSInfo
      fread( &BPB_FSInfo, 2, 1, fp);

      if( fp != NULL)
      {
        printf("CURRENT WORKING FILE: %s\n", token[1]);
        continue;
      }

      if( fp == NULL)
      {
        printf("Error: File system image not found.\n");
        continue;
      }

    } 

    // how the effffff is this giving a seg fault
    if( (strcmp( token[0], "close") == 0) )
    {
      if( fp == NULL)
      {
        printf("Error: File system not open.\n");
        continue;
      }
      else //nice way to call fclose() and test if it closed correctly
      {
      fclose(fp);
      fp = NULL;
      //printf("ERROR IN CLOSING THE FILE\n");
      continue;
      }
    }


    if( strcmp(token[0], "quit") == 0) // if the user will type in 'quit', the program will exit(0);
    {
      free( head_ptr); // i think this finally helps with the problem of having to type 'quit' or 'exit' multiple times
      if( fp != NULL)
      {
        fclose(fp);
      }
      //fclose(fp);
      exit(0);
    }

    if( strcmp(token[0], "exit") == 0) // if the user will type in 'exit', the program will exit(0);
    {
      free( head_ptr);
      if( fp != NULL)
      {
        fclose(fp);
      }
      //fclose(fp);
      exit(0);
    }

    if( (fp != NULL) && (strcmp( token[0], "info") == 0))
    {

      if( (strcmp( token[0] , "info") == 0) && fp != NULL)
      {
       
        printf("BPB_BytsPerSec  HEX: %-8x  | DECIMAL: %-8d\n", BPB_BytsPerSec, BPB_BytsPerSec); //1
        printf("BPB_FatsZ32     HEX: %-8x  | DECIMAL: %-8d\n", BPB_FatsZ32, BPB_FatsZ32); //2
        printf("BPB_SecPerClus  HEX: %-8x  | DECIMAL: %-8d\n", BPB_SecPerClus, BPB_SecPerClus); //3
        printf("BPB_RsvdSecCnt  HEX: %-8x  | DECIMAL: %-8d\n", BPB_RsvdSecCnt, BPB_RsvdSecCnt); //4 
        printf("BPB_NumFATS     HEX: %-8x  | DECIMAL: %-8d\n", BPB_NumFATs, BPB_NumFATs); //5
        printf("BPB_ExtFlags    HEX: %-8x  | DECIMAL: %-8d\n", BPB_ExtFlags, BPB_ExtFlags); //6
        printf("BPB_RootClus    HEX: %-8x  | DECIMAL: %-8d\n", BPB_RootClus, BPB_RootClus); //7
        printf("BPB_FSInfo      HEX: %-8x  | DECIMAL: %-8d\n", BPB_FSInfo, BPB_FSInfo); //8

        continue;

      }

    }


    //only works for one file for now... sorry professor
    if( (strcmp( token[0], "get") == 0) && token[1] == NULL )
    {
      printf("USAGE... get <filename> or get <filename> <new filename>\n");
      continue;
    }

    if( (strcmp(token[0], "get") == 0)  && token[1] != NULL)
    {
      char formattedFileName[12];
      convertTo8Dot3Format(token[1], formattedFileName);

      int currentCluster = BPB_RootClus;
      int dir_offset = LBAToOffset(currentCluster);
      int fileFound = 0;
      struct DirectoryEntry *entry = NULL;

      while (1) 
      {
        fseek(fp, dir_offset, SEEK_SET);
        fread(dir, sizeof(struct DirectoryEntry), 16, fp);

        for (int i = 0; i < 16; i++) 
        {
          if (strncmp(dir[i].DIR_Name, formattedFileName, 11) == 0) 
          {
            entry = &dir[i];
            fileFound = 1;
            break;
          }
        }

        if (fileFound) 
        {
          break;
        }

        currentCluster = NextLB(currentCluster);
        if (currentCluster == -1) 
        {
          break;
        }

          dir_offset = LBAToOffset(currentCluster);
      }

        if (!fileFound || entry == NULL) {
          printf("Error: File not found.\n");
          continue;
        }

        FILE *newFile = fopen(token[1], "w");
        if (newFile == NULL) 
        {
          printf("Error: Could not create new file.\n");
          continue;
        }

        int bytesToCopy = entry->DIR_FileSize;
        int cluster = entry->DIR_FirstClusterLow;
        int offset;

        while (bytesToCopy > 0) 
        {
          offset = LBAToOffset(cluster);
          fseek(fp, offset, SEEK_SET);

          char buffer[512];
          int bytesRead = (bytesToCopy > 512) ? 512 : bytesToCopy;
          fread(buffer, 1, bytesRead, fp);
          fwrite(buffer, 1, bytesRead, newFile);

          bytesToCopy -= bytesRead;
          cluster = NextLB(cluster);

          if (cluster == -1) 
          {
            break;
          }
        }

        fclose(newFile);
        //printf("File '%s' successfully copied to the local system.\n", token[1]);
        continue;
      }
    //is this cancer or no
    if( (strcmp( token[0], "put") == 0) && token[1] == NULL )
    {
      printf("USAGE... get <filename> or get <filename> <new filename>\n");
      continue;
    }

    if (strcmp(token[0], "stat") == 0 && token[1] != NULL) 
    {
      if (fp == NULL) 
      {
        printf("Error: File system image not opened.\n");
        continue;
      }

      char file[12];
      convertTo8Dot3Format(token[1], file);

      int currentCluster = BPB_RootClus;
      int dir_offset = LBAToOffset(currentCluster);
      int fileFound = 0;

      while (1) 
      {
        fseek(fp, dir_offset, SEEK_SET);
        fread(dir, sizeof(struct DirectoryEntry), 16, fp);

        for (int i = 0; i < 16; i++) 
        {
          if (strncmp(dir[i].DIR_Name, file, 11) == 0) 
          {
            printDirectoryEntryAttributes(&dir[i]);
            fileFound = 1;
            break;
          }
        }

        if ( fileFound) 
        {
          break;
        }

        currentCluster = NextLB(currentCluster);
        if ( currentCluster == -1) 
        {
          break;
        }

        dir_offset = LBAToOffset(currentCluster);
      }

      if ( !fileFound) 
      {
        printf("Error: File not found.\n");
      }

      continue;
   }


    if  (strcmp(token[0], "ls") == 0) 
    {
      if ( fp == NULL) 
      {
        printf("Error: File system image not opened.\n");
        continue;
      }

      int currentCluster = BPB_RootClus;
      int dir_offset = LBAToOffset(currentCluster);
      
      while (1) 
      {
        fseek( fp, dir_offset, SEEK_SET);
          
        fread( dir, sizeof(struct DirectoryEntry), 16, fp);

        int found_entries = 0;
        for (int i = 0; i < 16; i++) 
        {
          
          if ( dir[i].DIR_Name[0] == 0x00 || dir[i].DIR_Name[0] == 0xE5) 
          {
            continue;
          }
            
          if ( dir[i].DIR_Attr == 0x08 || (dir[i].DIR_Attr & 0x02)) 
          {
            continue;
          }

          char filename[12];
          memset(filename, 0, 12);
          strncpy(filename, dir[i].DIR_Name, 11);
          printf("%s\n", filename);

          found_entries++;
        }

        if (found_entries < 16)
        {
          break;
        }

        currentCluster = NextLB(currentCluster);
        if (currentCluster == -1) 
        {
          break;
        }

        dir_offset = LBAToOffset(currentCluster);
      }

      continue;
    }

    free( head_ptr );

  }

  return 0;
}
