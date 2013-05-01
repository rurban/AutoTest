#include <auto_generator.hpp>

static const char *use_list[] = {
    "use strict", "use warnings", 
    "use Test::More", "use Test::MockObject",
    NULL
};

TestCodeGenerator::TestCodeGenerator(void)
{
	fs = new FastSerializer();
	pkgs = new PackageMap();
}

bool TestCodeGenerator::existsPackage(const char *pkg_name)
{
	return pkgs->exists(pkg_name);
}

Package *TestCodeGenerator::getPackage(const char *pkg_name)
{
	return pkgs->get(pkg_name);
}

Package *TestCodeGenerator::addPackage(Package *pkg)
{
	pkgs->add(PackageMap::value_type(pkg->name, pkg));
	return pkg;
}

void TestCodeGenerator::dump(void)
{
	pkgs->dump();
}

void TestCodeGenerator::gen(void)
{
	const char *output_dirname = ".";
	char filename[MAX_FILE_NAME_SIZE] = {0};
	CHANGE_COLOR(GREEN);
	fprintf(stderr, "Test::AutoGenerator::gen start [./t/%s/]\n", output_dirname);
	CHANGE_COLOR(WHITE);
	PackageMap::iterator it = pkgs->begin();
	//FILE *fp = NULL;//stderr;
	for (;it != pkgs->end(); it++) {
		FILE *fp = NULL;
		Package *pkg = it->second;
		if (match(pkg->name, "main")) continue;
		snprintf(filename, MAX_FILE_NAME_SIZE, "./t/%s/%s.t", output_dirname, pkg->name);
		if (!fp) {
			if ((fp = fopen(filename, "w")) == NULL) {
				fprintf(stderr, "ERROR!!: file open error[%s], (%s)\n", filename, strerror(errno));
				exit(EXIT_FAILURE);
			}
		}
		//fprintf(stderr, "[%s]\n", filename);
        for (size_t i = 0; use_list[i] != NULL; i++) {
			fprintf(fp, "%s;\n", use_list[i]);
		}
		fprintf(fp, "\n");
		fprintf(fp, "use_ok('%s');\n", pkg->name);
		MethodMap::iterator mtd_it = pkg->mtds->begin();
		for (; mtd_it != pkg->mtds->end(); mtd_it++) {
			MethodList *mtd_list = mtd_it->second;
			MethodList::iterator list_it = mtd_list->begin();
			for (; list_it != mtd_list->end(); list_it++) {
				Method *mtd = *list_it;
				//fprintf(stderr, "mtd->name = [%s]\n", mtd->name);
				if (mtd->subname && match(mtd->subname, "main")) {
					continue;
				}
				fprintf(fp, "subtest '%s' => sub {\n", mtd->subname);
				bool comment_out_flag = false;
				CallFlowMap::iterator cfs_it = mtd->cfs->begin();
				for (; cfs_it != mtd->cfs->end(); cfs_it++) {
					CallFlowList *cf_list = cfs_it->second;
					CallFlowList::iterator cflist_it = cf_list->begin();
					for (; cflist_it != cf_list->end(); cflist_it++) {
						CallFlow *cf = *cflist_it;
						comment_out_flag = false;
						if (match(cf->to_stash, mtd->stash)) continue;
						if (!cf->ret || cf->is_xs) comment_out_flag = true;
						write_space(fp, 4, comment_out_flag);
						fprintf(fp, "Test::MockObject->fake_module('%s',\n", cf->to_stash);
						write_space(fp, 8, comment_out_flag);
						fprintf(fp, "%s => sub {\n", cf->to);
						write_space(fp, 12, comment_out_flag);
						if (cf->is_xs) {
							fprintf(fp, "%s\n", XS_ERROR_TEXT);
						} else if (!cf->ret) {
							fprintf(fp, "%s\n", TRACE_ERROR_TEXT);
						} else {
							fprintf(fp, "%s;\n", cf->ret);
						}
						write_space(fp, 8, comment_out_flag);
						fprintf(fp, "}\n");
						write_space(fp, 4, comment_out_flag);
						fprintf(fp, ");\n");
					}
				}
				comment_out_flag = false;
				if (mtd->stash && match(mtd->stash, "main")) {
					write_space(fp, 4, comment_out_flag);
					if (mtd->ret_type == TYPE_List) {
						fprintf(fp, "my @ret = %s(", mtd->subname);
					} else if (!mtd->ret) {
						write_space(fp, 4, comment_out_flag);
						fprintf(fp, "%s(", mtd->subname);
					} else {
						fprintf(fp, "my $ret = %s(", mtd->subname);
					}
				} else {
					write_space(fp, 4, comment_out_flag);
					if (mtd->ret_type == TYPE_List) {
						fprintf(fp, "my @ret = %s::%s(", mtd->stash, mtd->subname);
					} else if (!mtd->ret) {
						fprintf(fp, "%s::%s(", mtd->stash, mtd->subname);
					} else {
						fprintf(fp, "my $ret = %s::%s(", mtd->stash, mtd->subname);
					}
				}
				if (mtd->args) fprintf(fp, "%s", mtd->args);
				fprintf(fp, ");\n");
				if (mtd->ret) {
					switch (mtd->ret_type) {
					case TYPE_Int: case TYPE_Double:
						write_space(fp, 4, comment_out_flag);
						fprintf(fp, "ok($ret == %s, '%s');\n", mtd->ret, mtd->name);
						break;
					case TYPE_PtrInt: case TYPE_PtrDouble:
						write_space(fp, 4, comment_out_flag);
						if (find(mtd->ret, '\"')) {
							fprintf(fp, "ok($ret eq %s, '%s');\n", mtd->ret, mtd->name);
						} else {
							fprintf(fp, "ok($ret == %s, '%s');\n", mtd->ret, mtd->name);
						}
						break;
					case TYPE_String:
						write_space(fp, 4, comment_out_flag);
						fprintf(fp, "is($ret, %s, '%s');\n", mtd->ret, mtd->name);
						break;
					case TYPE_Hash: case TYPE_Array:
					case TYPE_Code: case TYPE_Object:
						write_space(fp, 4, comment_out_flag);
						fprintf(fp, "is_deeply($ret, %s, '%s');\n", mtd->ret, mtd->name);
						break;
					case TYPE_List:
						write_space(fp, 4, comment_out_flag);
						((char *)mtd->ret)[0] = '[';
						((char *)mtd->ret)[strlen(mtd->ret) - 1] = ']';
						fprintf(fp, "is_deeply(\\@ret, %s, '%s');\n", mtd->ret, mtd->name);
						break;
					default:
						write_space(fp, 4, comment_out_flag);
						fprintf(fp, "ok($ret == %s, '%s');\n", mtd->ret, mtd->name);
						break;
					}
				}
				fprintf(fp, "};\n");
				fprintf(fp, "\n");
			}
		}
		fprintf(fp, "\n");
		//fprintf(fp, "run_tests();\n");
		fprintf(fp, "done_testing();\n");
		fclose(fp);
	}
}

TestCodeGenerator::~TestCodeGenerator(void)
{
}
