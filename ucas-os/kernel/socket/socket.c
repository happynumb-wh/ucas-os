#include <os/socket.h>
#include <assert.h>
mutex_lock_t sock_lock;
struct socket sock[MAX_SOCK_NUM + 1];
// 初始化所有socket
int init_socket(){
    do_mutex_lock_init(&sock_lock);
    for (int i=1;i<=MAX_SOCK_NUM;i++) {
        memset(&sock[i],0,sizeof(struct socket));
        init_ring_buffer(&sock[i].data);
    }
}
// 分配并初始化一个socket，将对应的信息填入其中
int do_socket(int domain, int type, int protocol){
    int i;
    do_mutex_lock_acquire(&sock_lock);
    
    for (i=1;i<=MAX_SOCK_NUM;i++)
        if (!sock[i].used) break;
    if (i == MAX_SOCK_NUM + 1) {
        do_mutex_lock_release(&sock_lock);
        return -ENFILE;
    }
    sock[i].used = 1;
    sock[i].domain = domain;
    sock[i].status = SOCK_CLOSED;
    sock[i].type = type;
    sock[i].protocol = protocol;
    int new_fd_num = get_one_fd(current_running);
    if (new_fd_num == -1) return -ENFILE;
    int new_fd_index = get_fd_index(new_fd_num,current_running);
    current_running->pfd[new_fd_index].dev = DEV_SOCK;
    current_running->pfd[new_fd_index].sock_num = i;
    if (type & SOCK_CLOEXEC) current_running->pfd[new_fd_index].mode |= FD_CLOEXEC;
    if (type & SOCK_NONBLOCK) current_running->pfd[new_fd_index].flags |= O_NONBLOCK;
    do_mutex_lock_release(&sock_lock);
    return new_fd_num;
}
// 将socket的addr绑定在用户态指定的addr上
int do_bind(int sockfd, struct sockaddr *addr, socklen_t addrlen){
    
    assert(current_running->pfd[get_fd_index(sockfd,current_running)].dev == DEV_SOCK);
    assert(addrlen == sizeof(struct sockaddr));
    int sock_num = current_running->pfd[get_fd_index(sockfd,current_running)].sock_num;
    do_mutex_lock_acquire(&sock_lock);
    sock[sock_num].addr = addr;
    do_mutex_lock_release(&sock_lock);
    return 0;
}
//将socket的状态置为LISTEN
int do_listen(int sockfd, int backlog){
    
    assert(current_running->pfd[get_fd_index(sockfd,current_running)].dev == DEV_SOCK);
    assert(backlog<=MAX_WAIT_LIST);
    int sock_num = current_running->pfd[get_fd_index(sockfd,current_running)].sock_num;
    do_mutex_lock_acquire(&sock_lock);
    sock[sock_num].status = SOCK_LISTEN;
    sock[sock_num].backlog = backlog; 
    do_mutex_lock_release(&sock_lock);
    return 0;
}
// 连接至sockaddr对应的socket，即在所有socket中搜索sockaddr对应且状态为LISTENED的socket，
// 并将自己放入其的wait_list中。如果该socket是非阻塞的则直接返回，否则循环等待。
int do_connect(int sockfd, struct sockaddr *addr, socklen_t addrlen){
    
    assert(current_running->pfd[get_fd_index(sockfd,current_running)].dev == DEV_SOCK);
    if(addrlen != sizeof(struct sockaddr)) return -EINVAL;
    int sock_num = current_running->pfd[get_fd_index(sockfd,current_running)].sock_num;
    int i,j;
    do_mutex_lock_acquire(&sock_lock);
    for (i=1;i<=MAX_SOCK_NUM;i++){
        // printk("sock[%d]: addr = %x, status = %d\n",i,sock[i].addr->sin_addr,sock[i].status);
        if (!memcmp(addr,sock[i].addr,sizeof(struct sockaddr)) && sock[i].status == SOCK_LISTEN) break;
    }
    if (i == MAX_SOCK_NUM + 1) {
        do_mutex_lock_release(&sock_lock);
        return -ECONNREFUSED;
    }
    for (j=0;j<sock[i].backlog;j++)
        if (!sock[i].wait_list[j]) {
            sock[i].wait_list[j] = sock_num;
            break;
        }
    if (j==sock[i].backlog) {
        do_mutex_lock_release(&sock_lock);
        return -EWOULDBLOCK;
    }
    if (sock[sock_num].type & SOCK_NONBLOCK){
        sock[sock_num].status = SOCK_ACCEPTED;
    }
    else {
        while (sock[sock_num].status != SOCK_ACCEPTED){
            do_mutex_lock_release(&sock_lock);
            do_scheduler();
            do_mutex_lock_acquire(&sock_lock);
        }
    }
    do_mutex_lock_release(&sock_lock);
    return 0;
    }
// 用于接受socket等待列表中的连接，并创建一个新的socket指代该连接
int do_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen){
    
    assert(current_running->pfd[get_fd_index(sockfd,current_running)].dev == DEV_SOCK);
    assert(*addrlen == sizeof(struct sockaddr));
    int sock_num = current_running->pfd[get_fd_index(sockfd,current_running)].sock_num;
    assert(addr == sock[sock_num].addr);
    int i;
    do_mutex_lock_acquire(&sock_lock);
    for (i=0;i<sock[sock_num].backlog;i++) {
        if (sock[sock_num].wait_list[i]) break;
    }
    if (i==sock[sock_num].backlog) {
        do_mutex_lock_release(&sock_lock);
        return -EWOULDBLOCK;
    }
    sock[sock[sock_num].wait_list[i]].status = SOCK_ACCEPTED;
    sock[sock_num].wait_list[i] = 0;
    do_mutex_lock_release(&sock_lock);
    int new_sock = do_socket(sock[sock_num].domain,sock[sock_num].type,sock[sock_num].protocol);
    do_mutex_lock_acquire(&sock_lock);
    int new_fd_index = get_fd_index(new_sock,current_running);
    sock[current_running->pfd[new_fd_index].sock_num].status = SOCK_ESTABLISHED;
    do_mutex_lock_release(&sock_lock);
    return new_sock;
}
// 向sockaddr对应的socket发送数据（实际上是向其缓冲区写入数据）
ssize_t do_sendto(int sockfd, void *buf, size_t len, int flags, struct sockaddr *dest_addr, socklen_t addrlen){
    // for here, sockfd and flags are trivial because of the leak of network function
    
    assert(addrlen == sizeof(struct sockaddr));
    int i,j;
    do_mutex_lock_acquire(&sock_lock);
    for (i=1;i<=MAX_SOCK_NUM;i++)
        if (!memcmp(dest_addr,sock[i].addr,sizeof(struct sockaddr)) && 
            ((sock[i].protocol == IPPROTO_UDP) || (sock[i].protocol == IPPROTO_TCP && sock[i].status == SOCK_ESTABLISHED))) break;
    if (i == MAX_SOCK_NUM + 1) {
        do_mutex_lock_release(&sock_lock);
        return -ENOTCONN;
    }
    //printk("ready to write ringbuffer! ringbuffer:%lx, buf = %lx, len = %d\n",&sock[i].data,buf,len);
    int old_len = len;
    while (len > 0) {
		int write_len = write_ring_buffer(&sock[i].data,buf,len);
		buf += write_len;
		len -= write_len;
	}
    do_mutex_lock_release(&sock_lock);
    return old_len;
}
// 从src_addr地址读取数据（实际上是读取自己的缓冲区）
ssize_t do_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen){
    //for here, src_addr, addrlen and flags are trivial
    
    assert(current_running->pfd[get_fd_index(sockfd,current_running)].dev == DEV_SOCK);
    int sock_num = current_running->pfd[get_fd_index(sockfd,current_running)].sock_num;
    int i;
    do_mutex_lock_acquire(&sock_lock);
    int read_len = (len<ring_buffer_used(&sock[sock_num].data))?len:ring_buffer_used(&sock[sock_num].data);
    //printk("ready to read ringbuffer! ringbuffer:%lx, buf = %lx, len = %d\n",&sock[sock_num].data,buf,read_len);
	read_ring_buffer(&sock[sock_num].data, buf, read_len);
    do_mutex_lock_release(&sock_lock);
    return read_len;
}
// 关闭socket连接
int32_t close_socket(uint32_t sock_num){
    do_mutex_lock_acquire(&sock_lock);
    memset(&sock[sock_num],0,sizeof(struct socket));
    init_ring_buffer(&sock[sock_num].data);
    do_mutex_lock_release(&sock_lock);
    return 0;
}
// 获取socket的addr
int do_getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen){
    
    assert(current_running->pfd[get_fd_index(sockfd,current_running)].dev == DEV_SOCK);
    assert(*addrlen == sizeof(struct sockaddr));
    int sock_num = current_running->pfd[get_fd_index(sockfd,current_running)].sock_num;
    do_mutex_lock_acquire(&sock_lock);
    memcpy(addr,sock[sock_num].addr,sizeof(struct sockaddr));
    do_mutex_lock_release(&sock_lock);
    return 0;
}

int do_setsockopt(int sockfd, int level, int optname, void *optval, socklen_t optlen){
    return 0; //unused
}
