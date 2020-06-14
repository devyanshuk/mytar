### gnu-tar implementation in C


This tar implementation lists GNU tar archives created via the following invocation:

```console
devyanshu@10:~$ touch 1.txt 2.txt 3.txt
devyanshu@10:~$ tar -f archive.tar -c *.txt
```

##### Compiling the program:

```console
devyanshu@10:~$ gcc -Wall -Wextra -o mytar mytar.c
```

##### The user can search for a file in the archive by typing:

```console
devyanshu@10:~$ ./mytar -f archive.tar -t 1.txt
1.txt
```
##### Listing the whole contents of the archive:

```console
devyanshu@10:~$ ./mytar -f archive.tar -t
1.txt
2.txt
3.txt
```

If a file is not present in the archive, the program displays an error message

```console
devyanshu@10:~$ ./mytar -f archive.tar -t 1.txt 6.txt
1.txt
mytar: 6.txt: Not found in archive
mytar: Exiting with failure status due to previous errors
devyanshu@10:~$	echo $?
2
```
