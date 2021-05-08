// 주의사항
// 1. blockmap.h에 정의되어 있는 상수 변수를 우선적으로 사용해야 함
// 2. blockmap.h에 정의되어 있지 않을 경우 본인이 이 파일에서 만들어서 사용하면 됨
// 3. 필요한 data structure가 필요하면 이 파일에서 정의해서 쓰기 바람(blockmap.h에 추가하면 안됨)

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include "blockmap.h"
// 필요한 경우 헤더 파일을 추가하시오.

//
// flash memory를 처음 사용할 때 필요한 초기화 작업, 예를 들면 address mapping table에 대한
// 초기화 등의 작업을 수행한다. 따라서, 첫 번째 ftl_write() 또는 ftl_read()가 호출되기 전에
// file system에 의해 반드시 먼저 호출이 되어야 한다.
//
//

int amt[DATABLKS_PER_DEVICE][2];
int freeblock;

void ftl_open()
{
	char *buf;
	int lbn;
	int lsn;
	
	buf = (char *)malloc(PAGE_SIZE);
	freeblock = DATABLKS_PER_DEVICE;

	for (int i=0; i<DATABLKS_PER_DEVICE; i++){
		amt[i][0] = i;
		amt[i][1] = -1;
	} 
	for(int i=0; i<=DATABLKS_PER_DEVICE; i++){
		dd_read(i*PAGES_PER_BLOCK, buf);
		memcpy(&lbn, buf+SECTOR_SIZE, 4);
		if(lbn >= 0){
			amt[lbn][1]=i;
		}
	}
	/*for(int i=0; i<DATABLKS_PER_DEVICE; i++){
		printf("%d %d \n", amt[i][0], amt[i][1]);
	}*/

	free(buf);
	// address mapping table 초기화 또는 복구
	// free block's pbn 초기화
    	// address mapping table에서 lbn 수는 DATABLKS_PER_DEVICE 동일
	
	return;
}

//
// 이 함수를 호출하는 쪽(file system)에서 이미 sectorbuf가 가리키는 곳에 512B의 메모리가 할당되어 있어야 함
// (즉, 이 함수에서 메모리를 할당 받으면 안됨)
//
void ftl_write(int lsn, char *sectorbuf)
{
	int lbn = lsn / PAGES_PER_BLOCK;
	int pbn = amt[lbn][1];
	int offset = lsn % PAGES_PER_BLOCK;
	char *pagebuf;
	
	//printf("lsn=%d lbn=%d pbn=%d offset=%d\n", lsn, lbn, pbn, offset);

	pagebuf = (char *)malloc(PAGE_SIZE);
	
	if(pbn == -1){ //만약 최초 등록인 경우
		amt[lbn][1] = lbn;
		memcpy(pagebuf, sectorbuf, SECTOR_SIZE);		
        	memcpy(pagebuf+SECTOR_SIZE, &lbn, 4);
		dd_write(lbn*PAGES_PER_BLOCK, pagebuf);
	        memcpy(pagebuf+SECTOR_SIZE+4, &lsn, 4);
		dd_write(lbn*PAGES_PER_BLOCK+offset, pagebuf);
	}
	else{ //업데이트일 경우
		memcpy(pagebuf, sectorbuf, SECTOR_SIZE);
		memcpy(pagebuf+SECTOR_SIZE, &lbn, 4);
		dd_write(freeblock*PAGES_PER_BLOCK, pagebuf);
		memcpy(pagebuf+SECTOR_SIZE+4, &lsn, 4);
		dd_write(freeblock*PAGES_PER_BLOCK+offset, pagebuf);
		dd_erase(pbn);
		amt[lbn][1] = freeblock;
		freeblock = pbn;	
	}
	free(pagebuf);
	return;
}

//
// 이 함수를 호출하는 쪽(file system)에서 이미 sectorbuf가 가리키는 곳에 512B의 메모리가 할당되어 있어야 함
// (즉, 이 함수에서 메모리를 할당 받으면 안됨)
//
void ftl_read(int lsn, char *sectorbuf)
{
	int lbn = lsn / PAGES_PER_BLOCK;
	int pbn = amt[lbn][1] ;
	int offset = lsn % PAGES_PER_BLOCK;
	int ppn = pbn*PAGES_PER_BLOCK+offset;
	char *pagebuf;

	pagebuf = (char *)malloc(PAGE_SIZE);
	dd_read(ppn, pagebuf);
	memcpy(sectorbuf, pagebuf, SECTOR_SIZE);
	//printf("%s \n", sectorbuf); test print
	
	free(pagebuf);
	return;
}

void ftl_print()
{
	printf("lbn pbn\n");
	for(int i=0; i<DATABLKS_PER_DEVICE; i++){
                printf("%d %d \n", amt[i][0], amt[i][1]);
        }
	printf("free block's pbn=%d\n", freeblock);
	return;
}
