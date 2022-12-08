#include <err.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <git2.h>

static git_repository *repo;

static const char *relpath = "";

static char description[255] = "repos";
static char *name = "";
static char owner[255];
static char category[255];

void joinpath(char *buf, size_t bufsiz, const char *path, const char *path2)
{
    int r;

    r = snprintf(buf, bufsiz, "%s%s%s",
            path, path[0] && path[strlen(path) - 1] != '/' ? "/" : "", path2);
    if (r < 0 || (size_t)r >= bufsiz)
        errx(1, "path truncated: '%s%s%s'",
                path, path[0] && path[strlen(path) - 1] != '/' ? "/" : "", path2);
}

/* Percent-encode, see RFC3986 section 2.1. */
void percentencode(FILE *fp, const char *s, size_t len)
{
    static char tab[] = "0123456789ABCDEF";
    unsigned char uc;
    size_t i;

    for (i = 0; *s && i < len; s++, i++) {
        uc = *s;
        /* NOTE: do not encode '/' for paths */
        if (uc < '/' || uc >= 127 || (uc >= ':' && uc <= '@') || uc == '[' || uc == ']') {
            putc('%', fp);
            putc(tab[(uc >> 4) & 0x0f], fp);
            putc(tab[uc & 0x0f], fp);
        } else {
            putc(uc, fp);
        }
    }
}

/* Escape characters below as HTML 2.0 / XML 1.0. */
void xmlencode(FILE *fp, const char *s, size_t len)
{
    size_t i;

    for (i = 0; *s && i < len; s++, i++) {
        switch(*s) {
            case '<':  fputs("&lt;",   fp); break;
            case '>':  fputs("&gt;",   fp); break;
            case '\'': fputs("&#39;" , fp); break;
            case '&':  fputs("&amp;",  fp); break;
            case '"':  fputs("&quot;", fp); break;
            default:   putc(*s, fp);
        }
    }
}

void printtimeshort(FILE *fp, const git_time *intime)
{
    struct tm *intm;
    time_t t;
    char out[32];

    t = (time_t)intime->time;
    if (!(intm = gmtime(&t)))
        return;
    strftime(out, sizeof(out), "%Y-%m-%d %H:%M", intm);
    fputs(out, fp);
}

    void
writeheader(FILE *fp)
{
    fputs("<!doctype html>\n"
            "<html>\n<head>\n"
            "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />\n"
            "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" />\n"
            "<title>rj1 > repos</title>", fp);
    fprintf(fp, "<link rel=\"stylesheet\" type=\"text/css\" href=\"/css/style.css\">\n"
            "<link rel=\"icon\" href=\"/favicon.svg\">\n", relpath);
    fputs("</head>\n<body id=\"home\">\n<div class=\"content\">\n<header>\n"
          "<div class=\"main\">\n<a href=\"/\"><img src=\"/img/rj1.svg\" alt=\"rj1\" width=\"50\"></a>\n</div>\n<nav>\n"
          "<a href=\"/\">home</a>\n<a href=\"/notes\">notes</a>\n<a href=\"/repos\">repos</a>\n<a href=\"/notes/index.xml\">rss</a>\n"
          "</nav>\n</header>\n<h1>", fp);
    xmlencode(fp, description, strlen(description));
    fputs("</h1>\n<div id=\"content\">\n"
            "<table id=\"index\"><thead>\n"
            "<tr><th><b>name</b></th><th><b>description</b></th><th><b>last commit</b></th></tr>"
            "</thead><tbody>\n", fp);
}

void writefooter(FILE *fp)
{
    fputs("</tbody>\n</table>\n"
        "<footer>\n<hr>\n<div class=\"meta\">\n"
        "｢mail: <a href=\"mailto:rj1@riseup.net\">rj1@riseup.net</a>｣ "
        "｢irc: <a href=\"ircs:\/\/internetrelaychat.net:6697\">rj1@internetrelaychat.net</a>｣ "
        "｢gh: <a href=\"https://github.com/rj1\">rj1</a>｣ "
        "｢pgp: <a href=\"/gpg.txt\">F0:42:A0:B6:CB:41:FD:A2</a>｣</div>"
        "</div>\n</footer>\n</div>\n</body>\n</html>\n", fp);
}


int writelog(FILE *fp)
{
    git_commit *commit = NULL;
    const git_signature *author;
    git_revwalk *w = NULL;
    git_oid id;
    char *stripped_name = NULL, *p;
    int ret = 0;

    git_revwalk_new(&w, repo);
    git_revwalk_push_head(w);

    if (git_revwalk_next(&id, w) ||
            git_commit_lookup(&commit, repo, &id)) {
        ret = -1;
        goto err;
    }

    author = git_commit_author(commit);

    /* strip .git suffix */
    if (!(stripped_name = strdup(name)))
        err(1, "strdup");
    if ((p = strrchr(stripped_name, '.')))
        if (!strcmp(p, ".git"))
            *p = '\0';

    fputs("<tr class=\"repo\"><td><a href=\"", fp);
    percentencode(fp, stripped_name, strlen(stripped_name));
    fputs("/\">", fp);
    xmlencode(fp, stripped_name, strlen(stripped_name));
    fputs("</a></td><td>", fp);
    xmlencode(fp, description, strlen(description));
    fputs("</td><td>", fp);
    if (author)
        printtimeshort(fp, &(author->when));
    fputs("</td></tr>\n", fp);

    git_commit_free(commit);
err:
    git_revwalk_free(w);
    free(stripped_name);

    return ret;
}

int main(int argc, char *argv[])
{
    FILE *fp;
    char path[PATH_MAX], repodirabs[PATH_MAX + 1];
    const char *repodir;
    int i, ret = 0, tmp;

    if (argc < 2) {
        fprintf(stderr, "%s [repodir...]\n", argv[0]);
        return 1;
    }

    git_libgit2_init();

#ifdef __OpenBSD__
    if (pledge("stdio rpath", NULL) == -1)
        err(1, "pledge");
#endif

    writeheader(stdout);

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-c")) {
            i++;
            if (i == argc)
                err(1, "missing argument");
            repodir = argv[i];
            fputs("<tr class=\"cat\"><td>", stdout);
            xmlencode(stdout, repodir, strlen(repodir));
            fputs("</td><td></td><td></td></tr>\n", stdout);
            continue;
        }

        repodir = argv[i];
        if (!realpath(repodir, repodirabs))
            err(1, "realpath");

        if (git_repository_open_ext(&repo, repodir,
                    GIT_REPOSITORY_OPEN_NO_SEARCH, NULL)) {
            fprintf(stderr, "%s: cannot open repository\n", argv[0]);
            ret = 1;
            continue;
        }

        /* use directory name as name */
        if ((name = strrchr(repodirabs, '/')))
            name++;
        else
            name = "";

        /* read description or .git/description */
        joinpath(path, sizeof(path), repodir, "description");
        if (!(fp = fopen(path, "r"))) {
            joinpath(path, sizeof(path), repodir, ".git/description");
            fp = fopen(path, "r");
        }
        description[0] = '\0';
        if (fp) {
            if (!fgets(description, sizeof(description), fp))
                description[0] = '\0';
            tmp = strlen(description);
            if (tmp > 0 && description[tmp-1] == '\n')
                description[tmp-1] = '\0';
            fclose(fp);
        }

        /* read owner or .git/owner */
        joinpath(path, sizeof(path), repodir, "owner");
        if (!(fp = fopen(path, "r"))) {
            joinpath(path, sizeof(path), repodir, ".git/owner");
            fp = fopen(path, "r");
        }
        owner[0] = '\0';
        if (fp) {
            if (!fgets(owner, sizeof(owner), fp))
                owner[0] = '\0';
            owner[strcspn(owner, "\n")] = '\0';
            fclose(fp);
        }

        writelog(stdout);
    }

    writefooter(stdout);

    /* cleanup */
    git_repository_free(repo);
    git_libgit2_shutdown();

    return ret;
}
