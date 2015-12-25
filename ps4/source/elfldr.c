#include "common.h"
#include "elfldr.h"

/* Defines */

#define elfRelocationSymbol __ELFN(R_SYM)
#define elfRelocationType __ELFN(R_TYPE)
#define elfRelocationInfo __ELFN(R_INFO)

#define elfSymbolBind __ELFN(ST_BIND)
#define elfSymbolType __ELFN(ST_TYPE)
#define elfSymbolInfo __ELFN(ST_INFO)

#define elfIsElf(e) IS_ELF(*elfHeader(e)) // FIXME: Null deref

#define elfClass(e) (e == NULL ? 0 : e->data[4])
#define elfEncoding(e) (e == NULL ? 0 : e->data[5])
#define elfVersion(e) (e == NULL ? 0 : e->data[6])
#define elfABI(e) (e == NULL ? 0 : e->data[7])

/* Constants */

enum{ ElfMaximalStringLength = 4096 };

/* --- elf header --- */

ElfHeader *elfHeader(Elf *elf)
{
	if(!elf)
		return NULL;
	return (ElfHeader *)elf->data;
}

uint64_t elfEntry(Elf *elf)
{
	if(!elf)
		return 0;
	ElfHeader *h = elfHeader(elf);
	if(!h)
		return 0;
	return h->e_entry;
}

uint64_t elfLargestAlignment(Elf *elf)
{
	uint16_t index = 0;
	uint64_t alignment = 0;

	while(1)
	{
		ElfProgram *h = elfProgram(elf, &index, ElfProgramAttributeType, PT_LOAD);
		if(!h)
			break;
		if(alignment < h->p_align)
			alignment = h->p_align;
		++index;
	}
	return alignment;
}

uint64_t elfMemorySize(Elf *elf)
{
	ElfSection *sections;
	ElfProgram *programs;

	uint16_t size;
	uint16_t length;
	uint16_t index;

	uint64_t memorySize = 0;

	if(!elf)
		return 0;

	programs = elfPrograms(elf, &size, &length);
	if(programs)
	{
		for(index = 0; index < length; ++index)
		{
			ElfProgram *s = (ElfProgram *)((uint8_t *)programs + index * size);
			if(memorySize < s->p_paddr + s->p_memsz)
				memorySize = s->p_paddr + s->p_memsz;
		}
	}
	else
	{
		length = 0;
		sections = elfSections(elf, &size, &length);
		if(!sections)
			return 0;
		for(index = 0; index < length; ++index)
		{
			ElfSection *s = (ElfSection *)((uint8_t *)sections + index * size);
			if(memorySize < s->sh_addr + s->sh_size)
				memorySize = s->sh_addr + s->sh_size;
		}
	}

	return memorySize;
}

/* --- elf section header --- */

char *elfSectionStrings(Elf *elf, uint64_t *size)
{
	ElfHeader *h;
	uint16_t i;
	ElfSection *s;
 	h = elfHeader(elf);
	i = h->e_shstrndx;
	s = elfSection(elf, &i, ElfSectionAttributeNone, 0);
	if(size)
		*size = s->sh_size;
	return (char *)elf->data + s->sh_offset;
}

uint64_t elfSectionAttribute(ElfSection *elfSection, ElfSectionAttribute attribute)
{
	switch(attribute)
	{
		case ElfSectionAttributeName:
			return elfSection->sh_name;
		case ElfSectionAttributeType:
			return elfSection->sh_type;
		case ElfSectionAttributeFlags:
			return elfSection->sh_flags;
		case ElfSectionAttributeAddress:
			return elfSection->sh_addr;
		case ElfSectionAttributeOffset:
			return elfSection->sh_offset;
		case ElfSectionAttributeSize:
			return elfSection->sh_size;
		case ElfSectionAttributeLink:
			return elfSection->sh_link;
		case ElfSectionAttributeInfo:
			return elfSection->sh_info;
		case ElfSectionAttributeMemoryAlignment:
			return elfSection->sh_addralign;
		case ElfSectionAttributeEntrySize:
			return elfSection->sh_entsize;
		default:
			break;
	}
	return 0;
}

ElfSection *elfSections(Elf *elf, uint16_t *size, uint16_t *length)
{
	ElfHeader *h;

	if(!elf)
		return NULL;

	h = elfHeader(elf);

	if(h->e_shoff == 0)
		return NULL;

	if(size != NULL)
		*size = h->e_shentsize;
	if(length != NULL)
		*length = h->e_shnum;

	return (ElfSection *)(elf->data + h->e_shoff);
}

ElfSection *elfSection(Elf *elf, uint16_t *index, ElfSectionAttribute attribute, uint64_t value)
{
	uint16_t size;
	uint16_t length;
	ElfSection *h, *t;
	uint16_t i = 0;

	if(!index)
 		index = &i;

	h = elfSections(elf, &size, &length);

	if(!h)
		return NULL;

	for(; *index < length; ++(*index))
	{
		t = (ElfSection *)((uint8_t *)h + *index * size);
		if(attribute == ElfSectionAttributeNone || elfSectionAttribute(t, attribute) == value)
			return t;
	}

	return NULL;
}

ElfSection *elfSectionByName(Elf *elf, char *name)
{
	uint64_t size;
	char *mem = elfSectionStrings(elf, &size);

	uint32_t offset = elfStringToOffset(mem, size, name);
	ElfSection *sh = elfSection(elf, NULL, ElfSectionAttributeName, offset);

	return sh;
}

/* --- elf program header --- */

uint64_t elfProgramAttribute(ElfProgram *elfProgram, ElfProgramAttribute attribute)
{
	switch(attribute)
	{
		case ElfProgramAttributeType:
			return elfProgram->p_type;
		case ElfProgramAttributeFlags:
			return elfProgram->p_flags;
		case ElfProgramAttributeOffset:
			return elfProgram->p_offset;
		case ElfProgramAttributeVirtualAddress:
			return elfProgram->p_vaddr;
		case ElfProgramAttributePhysicalAddress:
			return elfProgram->p_paddr;
		case ElfProgramAttributeFileSize:
			return elfProgram->p_filesz;
		case ElfProgramAttributeMemorySize:
			return elfProgram->p_memsz;
		case ElfProgramAttributeAlignment:
			return elfProgram->p_align;
		default:
			break;
	}
	return 0;
}

ElfProgram *elfPrograms(Elf *elf, uint16_t *size, uint16_t *length)
{
	ElfHeader *h;

	if(!elf)
		return NULL;

	h = elfHeader(elf);

	if(h->e_phoff == 0)
		return NULL;

	if(size != NULL)
		*size = h->e_phentsize;
	if(length != NULL)
		*length = h->e_phnum;

	return (ElfProgram *)(elf->data + h->e_phoff);
}

ElfProgram *elfProgram(Elf *elf, uint16_t *index, ElfProgramAttribute attribute, uint64_t value)
{
	uint16_t size;
	uint16_t length;
	ElfProgram *h, *t;
	uint16_t i = 0;

	if(!index)
 		index = &i;

	h = elfPrograms(elf, &size, &length);

	if(!h)
		return NULL;

	for(; *index < length; ++(*index))
	{
		t = (ElfProgram *)((uint8_t *)h + *index * size);
		if(attribute == ElfProgramAttributeNone || elfProgramAttribute(t, attribute) == value)
			return t;
	}

	return NULL;
}

/* --- elf dynamic section --- */

uint64_t elfDynamicAttribute(ElfDynamic *elfDynamic, ElfDynamicAttribute attribute)
{
	switch(attribute)
	{
		case ElfDynamicAttributeTag:
			return elfDynamic->d_tag;
		case ElfDynamicAttributeValue:
			return elfDynamic->d_un.d_val;
		case ElfDynamicAttributePointer:
			return elfDynamic->d_un.d_ptr;
		default:
			break;
	}
	return 0;
}

ElfDynamic *elfDynamics(Elf *elf, uint16_t *size, uint16_t *length)
{
	ElfSection *h;
	ElfProgram *h2;

	if(!elf)
		return NULL;

	if((h = elfSection(elf, NULL, ElfSectionAttributeType, SHT_DYNAMIC)))
	{
		if(size != NULL)
			*size = h->sh_entsize;
		if(length != NULL)
			*length = h->sh_size / h->sh_entsize;

		return (ElfDynamic *)(elf->data + h->sh_offset);
	}
	else if((h2 = elfProgram(elf, NULL, ElfProgramAttributeType, SHT_DYNAMIC)))
	{
		if(size != NULL)
			*size = sizeof(ElfDynamic);
		if(length != NULL)
			*length = h2->p_filesz / sizeof(ElfDynamic);

		return (ElfDynamic *)(elf->data + h2->p_offset);
	}

	return NULL;
}

ElfDynamic *elfDynamic(Elf *elf, uint16_t *index, ElfDynamicAttribute attribute, uint64_t value)
{
	uint16_t size;
	uint16_t length;
	ElfDynamic *h, *t;
	uint16_t i = 0;

	if(!index)
 		index = &i;

	h = elfDynamics(elf, &size, &length);

	if(!h)
		return NULL;

	for(; *index < length; ++(*index))
	{
		t = (ElfDynamic *)((uint8_t *)h + *index * size);
		if(attribute == ElfDynamicAttributeNone || elfDynamicAttribute(t, attribute) == value)
			return t;
	}

	return NULL;
}

/* --- elf string tables --- */

char *elfStringFromIndex(char *mem, uint64_t size, uint32_t index)
{
	uint64_t i, j = 0;

	if(!mem)
		return NULL;

	if(index == 0)
		return mem;

	for(i = 0; i < size - 1; ++i)
		if(mem[i] == '\0' && ++j == index)
			return mem + i + 1;

	return NULL;
}

char *elfStringFromOffset(char *mem, uint64_t size, uint32_t offset)
{
	if(!mem || offset >= size)
		return NULL;

	return mem + offset;
}

uint32_t elfStringToOffset(char *mem, uint64_t size, char *str)
{
	uint64_t i, j;

	if(!str)
		return 0;

	for(i = 0; i < size; ++i)
	{
		for(j = 0; j < ElfMaximalStringLength && mem[i + j] == str[j]; ++j)
			if(str[j] == '\0')
				return i;
	}

	return 0;
}

uint32_t elfStringToIndex(char *mem, uint64_t size, char *str)
{
	uint64_t index, i, j;

	if(!str)
		return 0;

	index = 0;
	for(i = 0; i < size; ++i)
	{
		for(j = 0; j < ElfMaximalStringLength && mem[i + j] == str[j]; ++j)
			if(str[j] == '\0')
				return index;

		if(mem[i] == '\0')
			index++;
	}

	return 0;
}

/* --- elf relocations --- */

uint64_t elfAddendRelocationAttribute(ElfAddendRelocation *elfAddendRelocation, ElfAddendRelocationAttribute attribute)
{
	switch(attribute)
	{
		case ElfAddendRelocationAttributeInfo:
			return elfAddendRelocation->r_info;
		case ElfAddendRelocationAttributeOffset:
			return elfAddendRelocation->r_offset;
		case ElfAddendRelocationAttributeAddend:
			return elfAddendRelocation->r_addend;
		default:
			break;
	}
	return 0;
}

ElfAddendRelocation *elfAddendRelocations(Elf *elf, char *name, uint16_t *size, uint16_t *length)
{
	ElfSection *h;

 	h = elfSectionByName(elf, name);

	if(!h || h->sh_type != SHT_RELA)
		return NULL;

	if(size != NULL)
		*size = h->sh_entsize;
	if(length != NULL)
		*length = h->sh_size / h->sh_entsize;

	return (ElfAddendRelocation *)(elf->data + h->sh_offset);
}

// FIXME this is not performant, better to pass in the base ElfAddendRelocation *, size and length
/*
ElfAddendRelocation *elfAddendRelocation(Elf *elf, char *name, uint16_t *index, ElfAddendRelocationAttribute attribute, uint64_t value)
{
	uint16_t size;
	uint16_t length;
	ElfAddendRelocation *h, *t;
	uint16_t i = 0;

	if(!index)
 		index = &i;

	h = elfAddendRelocations(elf, name, &size, &length);

	if(!h)
		return NULL;

	for(; *index < length; ++(*index))
	{
		t = (ElfAddendRelocation *)((uint8_t *)h + *index * size);
		if(attribute == ElfAddendRelocationAttributeNone || elfAddendRelocationAttribute(t, attribute) == value)
			return t;
	}

	return NULL;
}
*/

/* --- elf symbols --- */

uint64_t elfSymbolAttribute(ElfSymbol *elfSymbol, ElfSymbolAttribute attribute)
{
	switch(attribute)
	{
		case ElfSymbolAttributeName:
			return elfSymbol->st_name;
		case ElfSymbolAttributeInfo:
			return elfSymbol->st_info;
		case ElfSymbolAttributeUnused:
			return elfSymbol->st_other;
		case ElfSymbolAttributeSectionIndex:
			return elfSymbol->st_shndx;
		case ElfSymbolAttributeValue:
			return elfSymbol->st_value;
		case ElfSymbolAttributeSize:
			return elfSymbol->st_size;
		default:
			break;
	}
	return 0;
}

ElfSymbol *elfSymbols(Elf *elf, char *name, uint16_t *size, uint16_t *length)
{
	ElfSection *h;

 	h = elfSectionByName(elf, name);

	if(!h || (h->sh_type != SHT_SYMTAB && h->sh_type != SHT_DYNSYM))
		return NULL;

	if(size != NULL)
		*size = h->sh_entsize;
	if(length != NULL)
		*length = h->sh_size / h->sh_entsize;

	return (ElfSymbol *)(elf->data + h->sh_offset);
}

/*
ElfSymbol *elfSymbol(Elf *elf, char *name, uint16_t *index, ElfSymbolAttribute attribute, uint64_t value)
{
	uint16_t size;
	uint16_t length;
	ElfSymbol *h, *t;
	uint16_t i = 0;

	if(!index)
 		index = &i;

	h = elfSymbols(elf, name, &size, &length);

	if(!h)
		return NULL;

	for(; *index < length; ++(*index))
	{
		t = (ElfSymbol *)((uint8_t *)h + *index * size);
		if(attribute == ElfSymbolAttributeNone || elfSymbolAttribute(t, attribute) == value)
			return t;
	}

	return NULL;
}*/

/* actions */

Elf *elfCreate(void *data, uint64_t size)
{
	Elf *elf, t;

	if(data == 0)
		return NULL;

	t.data = data;
	t.size = size;
	if(!elfIsElf(&t))
		return NULL;

 	elf = malloc(sizeof(Elf));
	elf->data = (uint8_t *)data;
	elf->size = size;

	return elf;
}

void *elfDestroy(Elf *elf)
{
	void *data;

	if(!elf)
		return NULL;

	data = elf->data;
	free(elf);

	return data;
}

void elfDestroyAndFree(Elf *elf)
{
	void *d = elfDestroy(elf);
	if(d)
		free(d);
}

/* ---  --- */

uint8_t elfLdrIsLoadable(Elf *elf)
{
	ElfHeader *h;

	if(!elfIsElf(elf))
		return 0;

	h = elfHeader(elf);

	return elfClass(elf) == ELFCLASS64 &&
		elfEncoding(elf) == ELFDATA2LSB &&
		elfVersion(elf) == EV_CURRENT &&
		(elfABI(elf) == ELFOSABI_SYSV || elfABI(elf) == ELFOSABI_FREEBSD) &&
		h->e_type == ET_DYN &&
		h->e_phoff != 0 &&
		h->e_shoff != 0 &&
		h->e_machine == EM_X86_64 &&
 		h->e_version == EV_CURRENT;
}

int elfLdrInstantiate(Elf *elf, void *memory)
{
	ElfSection *sections;
	ElfProgram *programs;

	uint16_t size;
	uint16_t length;
	uint16_t index;

	if(!elf)
		return -1;
	if(!memory)
		return -2;

	programs = elfPrograms(elf, &size, &length);
	if(programs)
	{
		for(index = 0; index < length; ++index)
		{
			ElfProgram *s = (ElfProgram *)((uint8_t *)programs + index * size);
			if(s->p_filesz)
				memcpy((char *)memory + s->p_paddr, elf->data + s->p_offset, s->p_filesz);
			if(s->p_memsz - s->p_filesz)
				memset((char *)memory + s->p_paddr + s->p_filesz, 0, s->p_memsz - s->p_filesz);
		}
	}
	else
	{
		length = 0;
		sections = elfSections(elf, &size, &length);
		if(!sections)
			return 0;
		for(index = 0; index < length; ++index)
		{
			ElfSection *s = (ElfSection *)((uint8_t *)sections + index * size);
			if(!(s->sh_flags & SHF_ALLOC))
				continue;
			if(s->sh_size)
				memcpy((char *)memory + s->sh_addr, elf->data + s->sh_offset, s->sh_size);
		}
	}

	return 1;
}

int elfLdrRelativeAddressIsExecutable(Elf *elf, uint64_t address)
{
	ElfSection *sections;
	ElfProgram *programs;

	uint16_t size;
	uint16_t length;
	uint16_t index;

	if(!elf)
		return -1;

	programs = elfPrograms(elf, &size, &length);
	if(programs)
	{
		for(index = 0; index < length; ++index)
		{
			ElfProgram *s = (ElfProgram *)((uint8_t *)programs + index * size);
			if(address >= s->p_paddr && address <= s->p_paddr + s->p_memsz)
				return s->p_flags & PF_X;
		}
	}
	else
	{
		length = 0;
		sections = elfSections(elf, &size, &length);
		if(!sections)
			return 0;
		for(index = 0; index < length; ++index)
		{
			ElfSection *s = (ElfSection *)((uint8_t *)sections + index * size);
			if(address >= s->sh_addr && address <= s->sh_addr + s->sh_size)
				return s->sh_flags & SHF_EXECINSTR;
		}
	}

	return 1;
}

int elfLdrRelocate(Elf *elf, void *writable, void *executable)
{
	int i, j;

	uint16_t relocationSize = 0;
	uint16_t relocationsLength = 0;
	ElfAddendRelocation *relocations;

	uint16_t dynamicSymbolSize = 0;
	uint16_t dynamicSymbolsLength = 0;
	ElfSymbol *dynamicSymbols;

	char *rel[2] = {".rela.dyn", ".rela.plt"};

	if(!elf || !writable || !executable)
		return 0;

	dynamicSymbols = elfSymbols(elf, ".dynsym", &dynamicSymbolSize, &dynamicSymbolsLength);
	//symbols = elfSymbols(elf, ".symtab", &symbolSize, &symbolsLength);

	for(j = 0; j < sizeof(rel) / sizeof(rel[0]); ++j)
	{
		relocationsLength = 0;
		relocations = elfAddendRelocations(elf, rel[j], &relocationSize, &relocationsLength);

		for(i = 0; i < relocationsLength; ++i)
		{
			ElfSymbol *symbol;
			ElfAddendRelocation *relocation = (ElfAddendRelocation *)(((uint8_t *)relocations) + relocationSize * i);
			uint16_t relocationType = (uint16_t)elfRelocationType(relocation->r_info);
			uint16_t relocationSymbol = (uint16_t)elfRelocationSymbol(relocation->r_info);
			uint64_t *offset = (uint64_t*)((char *)writable + relocation->r_offset);
			uint64_t value = 0;

			switch(relocationType)
			{
				case R_X86_64_RELATIVE:
					value = relocation->r_addend;
					break;
				case R_X86_64_64:
					symbol = (ElfSymbol *)(((uint8_t *)dynamicSymbols) + dynamicSymbolSize * relocationSymbol);
					value = symbol->st_value + relocation->r_addend;
					break;
				case R_X86_64_JMP_SLOT:
				case R_X86_64_GLOB_DAT:
					symbol = (ElfSymbol *)(((uint8_t *)dynamicSymbols) + dynamicSymbolSize * relocationSymbol);
					value = symbol->st_value;
					break;
				default:
					return 0;
			}

			if(elfLdrRelativeAddressIsExecutable(elf, value))
				*offset = (uint64_t)executable + value;
			else
				*offset = (uint64_t)writable + value;
		}
	}

	return 1;
}

int elfLdrLoad(Elf *elf, void *writable, void *executable)
{
	if(!elf)
		return -1;
 	if(writable == NULL)
		return -2;
 	if(executable == NULL)
		return -3;

	if(!elfLdrIsLoadable(elf))
		return -4;

	if(!elfLdrInstantiate(elf, writable))
		return -5;

	if(!elfLdrRelocate(elf, writable, executable))
		return -6;

	return 1;
}
