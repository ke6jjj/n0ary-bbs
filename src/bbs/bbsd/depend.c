#include <stdio.h>
#include <dirent.h>


void
dump_configurations(char *config_fn, char *dir)
{
	char w_flag = 'W';
	char cmdline[256];
	struct dirent *dp;
	DIR *dirp = opendir(dir);
	FILE *fp;	

	if((fp = fopen(config_fn, "r")) != NULL) {
		fclose(fp);
		sprintf(cmdline, "mv %s %s-", config_fn, config_fn);
		system(cmdline);
	}

	for(dp=readdir(dirp); dp!=NULL; dp=readdir(dirp)) {
		if(!strncmp(dp->d_name, "b_", 2)) {
			sprintf(cmdline, "%s/%s -%c -f %s",
				dir, dp->d_name, w_flag, config_fn);
			printf("%s\n", cmdline); fflush(stdout);
			system(cmdline);
			w_flag = 'w';
		}
	}
}

main(int argc, char *argv[])
{
	if(argc != 3) {
		printf("usage: %s exec_dir_path config_file\n", argv[0]);
		exit(1);
	}

	dump_configurations(argv[2], argv[1]);
}
