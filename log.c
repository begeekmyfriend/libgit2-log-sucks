/*
 * This example demonstrates the libgit2 rev walker APIs to roughly
 * simulate the output of 'git log -1 filename'.
 */

#include "common.h"

#include <sys/stat.h>
#include <sys/time.h>

/* log_state represents walker being configured while handling options */
struct log_state {
	git_repository *repo;
	const char *repodir;
	git_revwalk *walker;
        const char *ref;
	int hide;
	int sorting;
};
static struct log_state s;

/* Helper to find how many files in a commit changed from its nth parent. */
static int match_with_parent(git_commit *commit, int i, git_diff_options *opts)
{
	git_commit *parent;
	git_tree *a, *b;
	git_diff *diff;
	int ndeltas;

	check_lg2(
		git_commit_parent(&parent, commit, (size_t)i), "Get parent", NULL);
	check_lg2(git_commit_tree(&a, parent), "Tree for parent", NULL);
	check_lg2(git_commit_tree(&b, commit), "Tree for commit", NULL);
	check_lg2(
		git_diff_tree_to_tree(&diff, git_commit_owner(commit), a, b, opts),
		"Checking diff between parent and commit", NULL);

	ndeltas = (int)git_diff_num_deltas(diff);

        git_diff_free(diff);
        git_tree_free(a);
        git_tree_free(b);
        git_commit_free(parent);

        return ndeltas > 0;
}

/** Helper to format a git_time value like Git. */
static void print_time(const git_time *intime, const char *prefix)
{
	char sign, out[32];
	struct tm *intm;
	int offset, hours, minutes;
	time_t t;

	offset = intime->offset;
	if (offset < 0) {
		sign = '-';
		offset = -offset;
	} else {
		sign = '+';
	}

	hours   = offset / 60;
	minutes = offset % 60;

	t = (time_t)intime->time + (intime->offset * 60);

	intm = gmtime(&t);
	strftime(out, sizeof(out), "%a %b %e %T %Y", intm);

	printf("%s%s %c%02d%02d\n", prefix, out, sign, hours, minutes);
}

/** Helper to print a commit object. */
static void print_commit(git_commit *commit)
{
	char buf[GIT_OID_HEXSZ + 1];
	int i, count;
	const git_signature *sig;
	const char *scan, *eol;

	git_oid_tostr(buf, sizeof(buf), git_commit_id(commit));
	printf("commit %s\n", buf);

	if ((count = (int)git_commit_parentcount(commit)) > 1) {
		printf("Merge:");
		for (i = 0; i < count; ++i) {
			git_oid_tostr(buf, 8, git_commit_parent_id(commit, i));
			printf(" %s", buf);
		}
		printf("\n");
	}

	if ((sig = git_commit_author(commit)) != NULL) {
		printf("Author: %s <%s>\n", sig->name, sig->email);
		print_time(&sig->when, "Date:   ");
	}
	printf("\n");

	for (scan = git_commit_message(commit); scan && *scan; ) {
		for (eol = scan; *eol && *eol != '\n'; ++eol) /* find eol */;

		printf("    %.*s\n", (int)(eol - scan), scan);
		scan = *eol ? eol + 1 : NULL;
	}
	printf("\n");
}

/* Helper to get the latest commit of the specified file */
int git_show_last_commit(char *filename)
{
        git_oid oid;
        git_commit *commit = NULL;

        /* Set up pathspec. */
        git_diff_options diffopts = GIT_DIFF_OPTIONS_INIT;
        diffopts.pathspec.strings = &filename;
        diffopts.pathspec.count = 1;

        /* Use the revwalker to traverse the history. */
        check_lg2(git_revwalk_push_ref(s.walker, s.ref),
                        "Could not find repository reference", NULL);

        for (; !git_revwalk_next(&oid, s.walker); git_commit_free(commit)) {
                check_lg2(git_commit_lookup(&commit, s.repo, &oid),
                                "Failed to look up commit", NULL);

                int parents = (int)git_commit_parentcount(commit);
                int unmatched = parents;
                if (parents == 0) {
                        git_tree *tree;
                        git_pathspec *ps;
                        check_lg2(git_commit_tree(&tree, commit), "Get tree", NULL);
                        check_lg2(git_pathspec_new(&ps, &diffopts.pathspec),
                                        "Building pathspec", NULL);
                        if (git_pathspec_match_tree(
                                                NULL, tree, GIT_PATHSPEC_NO_MATCH_ERROR, ps) != 0)
                                unmatched = 1;
                        git_pathspec_free(ps);
                        git_tree_free(tree);
                } else {
                        int i;
                        for (i = 0; i < parents; ++i) {
                                if (match_with_parent(commit, i, &diffopts))
                                        unmatched--;
                        }
                }
                if (unmatched > 0)
                        continue;

                print_commit(commit);
                git_commit_free(commit);
                break;
        }
        git_revwalk_reset(s.walker);
        return 0;
}

int main(int argc, char **argv)
{
        struct stat st;
        struct timeval start, end;
        char filepath[1024], repodir[64];
        char *dirpath, *filename, *ref = NULL;

        if (argc != 3) {
                fprintf(stderr, "Usage: ./log dirpath filename\n");
                exit(-1);
        }

        dirpath = argv[1];
        filename = argv[2];
        strcpy(repodir, dirpath);
        strcat(repodir, "/.git");
        strcpy(filepath, dirpath);
        strcat(filepath, "/");
        strcat(filepath, filename);
        if (stat(filepath, &st) < 0) {
                fprintf(stderr, "Not valid file path: \"%s\"!\n", filepath);
                exit(-2);
        }

        memset(&s, 0, sizeof(s));
        s.sorting = GIT_SORT_TIME;
        s.hide = 0;
        s.repodir = strlen(repodir) > 0  ? repodir : "/tmp/git/.git";
        s.ref = ref ? ref : "refs/heads/master";

        /* Init libgit2 library */
        git_libgit2_init();

        /* Open repo. */
        check_lg2(git_repository_open_ext(&s.repo, s.repodir, 0, NULL),
                "Could not open repository", s.repodir);

	/* Create revwalker. */
        check_lg2(git_revwalk_new(&s.walker, s.repo),
                "Could not create revision walker", NULL);
        git_revwalk_sorting(s.walker, s.sorting);

        /* Show file's latest commit. */
        printf("filename: %s\n", filename);
        gettimeofday(&start, NULL);
        git_show_last_commit(filename);
        gettimeofday(&end, NULL);
        printf("time span: %ld(ms)\n", (end.tv_sec - start.tv_sec) * 1000 + \
                        (end.tv_usec - start.tv_usec) / 1000);

	git_revwalk_free(s.walker);
	git_repository_free(s.repo);
	git_libgit2_shutdown();

        return 0;
}
