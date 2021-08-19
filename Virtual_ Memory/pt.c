#include <stdio.h>
#include "os.h"
#define LEVELS 5

//declaration of helping functions//
_Bool is_valid(uint64_t pte);
short int move_page_pt(uint64_t vpn);
void destroy_mapping(uint64_t* ptr_to_pt,uint64_t vpn);
void create_mapping(uint64_t* ptr_to_pt,uint64_t vpn, uint64_t ppn);

void page_table_update(uint64_t pt, uint64_t vpn, uint64_t ppn){
	uint64_t* ptr_to_pt=phys_to_virt(((pt>>1)<<1)<<12);
	if (ppn==NO_MAPPING){
		if(page_table_query(pt,vpn) == NO_MAPPING)
			return;
		destroy_mapping(ptr_to_pt,vpn<<12);
		return;
	}
	create_mapping(ptr_to_pt,vpn<<12,ppn);
}

uint64_t page_table_query(uint64_t pt, uint64_t vpn){
	vpn<<=12;
	uint64_t* ptr_to_pt=phys_to_virt(((pt>>1)<<1)<<12);
	for(int i=0;i<LEVELS;++i,vpn<<=9){
		ptr_to_pt+=move_page_pt(vpn);
		if(!is_valid(*ptr_to_pt))           
			return NO_MAPPING;
		if(i==4)
			break;
		ptr_to_pt=phys_to_virt(*ptr_to_pt);
	}
	return *ptr_to_pt>>12; 
}

_Bool is_valid(uint64_t pte){
	return pte&1;
}

short int move_page_pt(uint64_t vpn){
	uint64_t mask=0x1FF;
	return ((mask<<48) & vpn)>>48;
}

void destroy_mapping(uint64_t* ptr_to_pt,uint64_t vpn){
	for(int i=0;i<LEVELS-1;i++,vpn<<=9)
		ptr_to_pt=phys_to_virt(*(ptr_to_pt+move_page_pt(vpn)));
	ptr_to_pt+=move_page_pt(vpn);
	*ptr_to_pt=(*ptr_to_pt>>1)<<1;
}

void create_mapping(uint64_t* ptr_to_pt,uint64_t vpn, uint64_t ppn){
	for(int i=0;i<LEVELS-1;i++,vpn<<=9){
		ptr_to_pt+=move_page_pt(vpn);
		if(!is_valid(*ptr_to_pt))
			*ptr_to_pt=(alloc_page_frame()<<12)|1;
		ptr_to_pt=phys_to_virt(*ptr_to_pt);
	}
	ptr_to_pt+=move_page_pt(vpn);
	*ptr_to_pt=((ppn<<12)|1);
}
