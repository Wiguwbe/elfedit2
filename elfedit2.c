
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <elf.h>

#define NAME_SIZE 128
#define NAME_STR_LEN NAME_SIZE-1
struct arg {
	char name[NAME_SIZE];	// should be enough
	long value;
	int name_len;
};

static int set1(FILE*, Elf64_Sym *sym, Elf64_Shdr *thdr, long value);
static int set2(FILE*, Elf64_Sym *sym, Elf64_Shdr *thdr, long value);
static int set4(FILE*, Elf64_Sym *sym, Elf64_Shdr *thdr, long value);
static int set8(FILE*, Elf64_Sym *sym, Elf64_Shdr *thdr, long value);

int main(int argc, char **argv)
{
	// read file
	// read variables to set:
	// 'VAR_NAME=<value>'
	// (only allow integers)
	// (assume 64bit)

	// look for .symtab

	if(argc < 3)
	{
		fprintf(stderr, "Usage: %s <elf file> <definition>...\n", *argv);
		return 1;
	}

	int arg_count = argc-2;
	struct arg *args = (struct arg*)malloc(arg_count*sizeof(struct arg));
	if(!args)
	{
		fprintf(stderr, "Failed to allocate memory\n");
		return 1;
	}

	// parse arguments
	struct arg *c_arg = args;
	for(int i=2;i<argc;i++)
	{
		char *eq = strchr(argv[i], '=');
		if(!eq)
		{
			fprintf(stderr, "Invalid argument: '%s' (expected an '=')\n", argv[i]);
			return 2;
		}
		int name_len = eq-argv[i];
		if(name_len > NAME_STR_LEN)
		{
			fprintf(stderr, "Variable name is too big (%d > %d, recompile with a bigger buffer if needed)\n", name_len, NAME_STR_LEN);
			return 2;
		}
		memcpy(c_arg->name, argv[i], name_len);
		c_arg->name[name_len] = 0;
		c_arg->name_len = name_len;
		char *p;
		long value = strtol(eq+1, &p, 0);
		if(*p != 0)
		{
			fprintf(stderr, "Invalid integer value: '%s'\n", eq+1);
			return 2;
		}
		c_arg->value = value;
		//fprintf(stderr, "Arg is '%s'=%ld\n", c_arg->name, c_arg->value);
		c_arg++;
	}

	FILE *file = fopen(argv[1], "r+b");
	if(!file)
	{
		fprintf(stderr, "Failed to open file\n");
		return 1;
	}

	// get into position
	Elf64_Ehdr elf_hdr;
	if(fread(&elf_hdr, sizeof(elf_hdr), 1, file) != 1)
	{
		fprintf(stderr, "Failed to read elf header\n");
		return 1;
	}

	// get section headers
	if(fseek(file, (long)elf_hdr.e_shoff, SEEK_SET))
	{
		fprintf(stderr, "Failed to seek file\n");
		return 1;
	}

	Elf64_Shdr sec_hdrs[elf_hdr.e_shnum];
	if(fread(sec_hdrs, sizeof(Elf64_Shdr), elf_hdr.e_shnum, file) != elf_hdr.e_shnum)
	{
		fprintf(stderr, "Failed to read section headers\n");
		return 1;
	}

	// find symtab
	Elf64_Shdr *shstrtab = sec_hdrs + elf_hdr.e_shstrndx;
	Elf64_Shdr *symtab = NULL;
	Elf64_Shdr *strtab = NULL;
	for(int i=0;i<elf_hdr.e_shnum;i++)
	{
		// look for ".symtab"/".strtab" (8 bytes)
		char name[8];
		if(fseek(file, (long)(shstrtab->sh_offset + sec_hdrs[i].sh_name), SEEK_SET))
		{
			fprintf(stderr, "Failed to seek file\n");
			return 1;
		}
		if(fread(name, 1, 8, file) != 8)
		{
			fprintf(stderr, "Failed to read\n");
			return 1;
		}
		if(!memcmp(name, ".symtab", 8))
			symtab = sec_hdrs+i;
		if(!memcmp(name, ".strtab", 8))
			strtab = sec_hdrs+i;
	}
	if(!symtab || !strtab)
	{
		fprintf(stderr, "Weirdly, this doesn't appear to have a .symtab/.strtab\n");
		return 1;
	}

	// now iterate the symbols
	int symtab_count = symtab->sh_size / symtab->sh_entsize;
	for(int i=0; i<symtab_count; i++)
	{
		// read sym entry
		Elf64_Sym sym;
		if(fseek(file, (long)(symtab->sh_offset+(i*symtab->sh_entsize)), SEEK_SET))
		{
			fprintf(stderr, "Failed to seek file\n");
			return 1;
		}
		if(fread(&sym, sizeof(sym), 1, file) != 1)
		{
			fprintf(stderr, "Failed to read file\n");
			return 1;
		}
		// check name against this one
		char name[NAME_SIZE];
		if(fseek(file, (long)(strtab->sh_offset+sym.st_name), SEEK_SET))
		{
			fprintf(stderr, "Failed to seek file\n");
			return 1;
		}
		// read until NULL terminator
		int j;
		for(j=0;j<NAME_SIZE;j++)
		{
			name[j] = getc(file);
			if(name[j] == 0)
				break;
		}
		if(j == NAME_SIZE)
		{
			fprintf(stderr, "Symtab entry name is too long (> %d, recompile with fitting values)\n", NAME_SIZE);
			return 1;
		}
		// and look for our arguments
		struct arg *c_arg = NULL;
		for(j=0;j<arg_count;j++)
		{
			if(!strcmp(args[j].name, name))
			{
				c_arg = args+j;
				break;
			}
		}
		if(!c_arg)
			continue;

		// set value
		int (*setter)(FILE*, Elf64_Sym*, Elf64_Shdr*, long);
		switch(sym.st_size)
		{
		case 1:
			setter = set1;
			break;
		case 2:
			setter = set2;
			break;
		case 4:
			setter = set4;
			break;
		case 8:
			setter = set8;
			break;
		default:
			fprintf(stderr, "Unhandled value size: %d (%s)\n", sym.st_size, name);
			return 1;
		}
		if(setter(file, &sym, sec_hdrs+sym.st_shndx, c_arg->value))
		{
			fprintf(stderr, "Failed to set value (%s)\n", name);
			return 1;
		}
		// next
	}

	// all good
	if(fflush(file))
	{
		fprintf(stderr, "Failed to flush file to disk\n");
		return 1;
	}
	if(fclose(file))
	{
		fprintf(stderr, "Failed to close file\n");
		return 1;
	}

	// memory shall be freed
	return 0;
}

int _set(FILE* file, Elf64_Sym *sym, Elf64_Shdr *shdr, void *data, int size)
{

	// seek there
	//fprintf(stderr, "Setting %d bytes at %x\n", size, sym->st_value - shdr->sh_addr + shdr->sh_offset);
	if(fseek(file, (long)(shdr->sh_offset + sym->st_value - shdr->sh_addr), SEEK_SET))
	{
		fprintf(stderr, "Failed to seek file\n");
		return 1;
	}

	// write
	if(fwrite(data, size, 1, file) != 1)
	{
		fprintf(stderr, "Failed to write value\n");
		return 1;
	}
	return 0;
}

int set1(FILE* file, Elf64_Sym *sym, Elf64_Shdr *shdr, long lval)
{
	// reduce size (keep sign)
	int8_t value = (int8_t)lval;
	return _set(file, sym, shdr, (void*)&value, 1);
}

int set2(FILE* file, Elf64_Sym *sym, Elf64_Shdr *shdr, long lval)
{
	int16_t value = (int16_t)lval;
	return _set(file, sym, shdr, &value, 2);
}

int set4(FILE* file, Elf64_Sym *sym, Elf64_Shdr *shdr, long lval)
{
	int32_t value = (int32_t)lval;
	return _set(file, sym, shdr, &value, 4);
}

int set8(FILE*file, Elf64_Sym *sym, Elf64_Shdr*shdr, long lval)
{
	int64_t value = (int64_t)lval;
	return _set(file, sym, shdr, &value, 8);
}
