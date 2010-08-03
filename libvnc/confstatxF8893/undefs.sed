/^[	 ]*#[	 ]*undef/!b
t clr
: clr
s,^\([	 ]*\)#\([	 ]*\)undef\([	 ][	 ]*\)LSTAT_FOLLOWS_SLASHED_SYMLINK$,\1#\2define\3LSTAT_FOLLOWS_SLASHED_SYMLINK 1,;t
s,^\([	 ]*\)#\([	 ]*\)undef\([	 ][	 ]*\)HAVE_STRFTIME$,\1#\2define\3HAVE_STRFTIME 1,;t
s,^\([	 ]*\)#\([	 ]*\)undef\([	 ][	 ]*\)HAVE_VPRINTF$,\1#\2define\3HAVE_VPRINTF 1,;t
s,^\([	 ]*\)#\([	 ]*\)undef\([	 ][	 ]*\)HAVE_UNISTD_H$,\1#\2define\3HAVE_UNISTD_H 1,;t
s,^\([	 ]*\)#\([	 ]*\)undef\([	 ][	 ]*\)HAVE_FORK$,\1#\2define\3HAVE_FORK 1,;t
s,^\([	 ]*\)#\([	 ]*\)undef\([	 ][	 ]*\)HAVE_VFORK$,\1#\2define\3HAVE_VFORK 1,;t
s,^\([	 ]*\)#\([	 ]*\)undef\([	 ][	 ]*\)HAVE_WORKING_VFORK$,\1#\2define\3HAVE_WORKING_VFORK 1,;t
s,^\([	 ]*\)#\([	 ]*\)undef\([	 ][	 ]*\)HAVE_WORKING_FORK$,\1#\2define\3HAVE_WORKING_FORK 1,;t
s,^\([	 ]*\)#\([	 ]*\)undef\([	 ][	 ]*\)HAVE_LIBNSL$,\1#\2define\3HAVE_LIBNSL 1,;t
s,^\([	 ]*\)#\([	 ]*\)undef\([	 ][	 ]*\)HAVE_FTIME$,\1#\2define\3HAVE_FTIME 1,;t
s,^\([	 ]*\)#\([	 ]*\)undef\([	 ][	 ]*\)HAVE_GETHOSTBYNAME$,\1#\2define\3HAVE_GETHOSTBYNAME 1,;t
s,^\([	 ]*\)#\([	 ]*\)undef\([	 ][	 ]*\)HAVE_GETHOSTNAME$,\1#\2define\3HAVE_GETHOSTNAME 1,;t
s,^\([	 ]*\)#\([	 ]*\)undef\([	 ][	 ]*\)HAVE_GETTIMEOFDAY$,\1#\2define\3HAVE_GETTIMEOFDAY 1,;t
s,^\([	 ]*\)#\([	 ]*\)undef\([	 ][	 ]*\)HAVE_INET_NTOA$,\1#\2define\3HAVE_INET_NTOA 1,;t
s,^\([	 ]*\)#\([	 ]*\)undef\([	 ][	 ]*\)HAVE_MEMMOVE$,\1#\2define\3HAVE_MEMMOVE 1,;t
s,^\([	 ]*\)#\([	 ]*\)undef\([	 ][	 ]*\)HAVE_MEMSET$,\1#\2define\3HAVE_MEMSET 1,;t
s,^\([	 ]*\)#\([	 ]*\)undef\([	 ][	 ]*\)HAVE_MMAP$,\1#\2define\3HAVE_MMAP 1,;t
s,^\([	 ]*\)#\([	 ]*\)undef\([	 ][	 ]*\)HAVE_MKFIFO$,\1#\2define\3HAVE_MKFIFO 1,;t
s,^\([	 ]*\)#\([	 ]*\)undef\([	 ][	 ]*\)HAVE_SELECT$,\1#\2define\3HAVE_SELECT 1,;t
s,^\([	 ]*\)#\([	 ]*\)undef\([	 ][	 ]*\)HAVE_SOCKET$,\1#\2define\3HAVE_SOCKET 1,;t
s,^\([	 ]*\)#\([	 ]*\)undef\([	 ][	 ]*\)HAVE_STRCHR$,\1#\2define\3HAVE_STRCHR 1,;t
s,^\([	 ]*\)#\([	 ]*\)undef\([	 ][	 ]*\)HAVE_STRCSPN$,\1#\2define\3HAVE_STRCSPN 1,;t
s,^\([	 ]*\)#\([	 ]*\)undef\([	 ][	 ]*\)HAVE_STRDUP$,\1#\2define\3HAVE_STRDUP 1,;t
s,^\([	 ]*\)#\([	 ]*\)undef\([	 ][	 ]*\)HAVE_STRERROR$,\1#\2define\3HAVE_STRERROR 1,;t
s,^\([	 ]*\)#\([	 ]*\)undef\([	 ][	 ]*\)HAVE_STRSTR$,\1#\2define\3HAVE_STRSTR 1,;t
s,^[	 ]*#[	 ]*undef[	 ][	 ]*[a-zA-Z_][a-zA-Z_0-9]*,/* & */,
