# CN2020-Homework2 - Simple Network Storage System
## Usage
```shell
$ make
```
產生 server 與 client 執行檔。
```shell
$ ./server <port>
$ ./client <ip>:<port> # another terminal
```
分別開啟 server 與 client

## Functions
- watch a video
    ```
    play <videofile>
    ```
- upload files
    ```
    put <filename>
    ```
- download files
    ```
    get <filename>
    ```
- list files
    ```
    ls
    ```