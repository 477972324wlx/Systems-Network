#include <string.h>
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <netinet/ip.h>  
#include <netinet/ip_icmp.h>  
#include <unistd.h>  
#include <signal.h>  
#include <pthread.h>
#include <arpa/inet.h>  
#include <errno.h>  
#include <sys/time.h>  
#include <stdio.h>  
#include <string.h> /*?bzero?*/  
#include <netdb.h>  

#define icmp_pptr icmp_hun.ih_pptr
#define icmp_gwaddr icmp_hun.ih_gwaddr
#define icmp_id icmp_hun.ih_idseq.icd_id
#define icmp_seq icmp_hun.ih_idseq.icd_seq
#define icmp_void icmp_hun.ih_void
#define icmp_pmvoid icmp_hun.ih_pmtu.ipm_void
#define icmp_nextmtu icmp_hun.ih_pmtu.ipm_nextmtu
#define icmp_num_addrs icmp_hun.ih_rtradv.irt_num_addrs
#define icmp_wpa icmp_hun.ih_rtradv.irt_wpa
#define icmp_lifetime icmp_hun.ih_rtradv.irt_lifetime

#define BUFFERSIZE 256

static int rawsock;
static pid_t pid;
static int alive, packet_send;
static unsigned char send_buff[4000];  

struct PingPacket{
    timeval tv_begin;
    timeval tv_end;
    short   seq;
    int     flag;
}packet[128];; 

PingPacket * icmp_findpacket(int seq);
unsigned short icmp_check(unsigned char * data, int len);
timeval tv_begin, tv_end, tv_interval;
static timeval icmp_tvsub(struct timeval end, struct timeval begin)  
{  
    struct timeval tv;  
    //计算差值  
    tv.tv_sec = end.tv_sec - begin.tv_sec;  
    tv.tv_usec = end.tv_usec - begin.tv_usec;  
    //如果接收的时间的usec值小于发送时的usec,从uesc域借位  
    if(tv.tv_usec < 0){  
        tv.tv_sec --;  
        tv.tv_usec += 1000000;  
    }  
  
    return tv;  
}
PingPacket* icmp_findpacket(int seq){
    PingPacket* res = 0;
    int tag = (seq == -1) ? 0 : seq;

    for (int i = 0 ; i < 128 ; ++i){
        if(packet[i].seq == tag){
            res = &packet[i];
            break;
        }
    }

    return res;
}
unsigned short icmp_checksum(unsigned char * data, int len){
    int sum = 0;
    int odd = len & 1;

    while(len & 0xfffe){
        sum  += *(unsigned short *) data;
        data += 2;
        len  -= 2;
    }
    if(odd){
        u_int16_t tmp = ((*data) << 8) & 0xff00;
        sum += tmp;
    }
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);

    return ~sum;
}

void icmp_pack(struct icmp * icmph, int seq, timeval * tv , int length){
    icmph->icmp_type = ICMP_ECHO;
    icmph->icmp_code = 0;
    icmph->icmp_cksum = 0;
    icmph->icmp_seq = seq;
    icmph->icmp_id = pid;
    for (int i = 0 ; i < length ; ++i){
        icmph->icmp_data[i] = i*2;
    }
    icmph->icmp_cksum = icmp_checksum((unsigned char *) icmph, length);

}

int icmp_unpack(char * buf, int len){
    int i, iphdrlen;
    ip*     iph = 0;
    icmp*   icmph = 0;
    int rtt;

    iph = (ip* )buf;
    iphdrlen = iph->ip_hl * 4;
    icmph = (icmp*) (buf + iphdrlen);
    len -= iphdrlen;

    if(len < 8){
        printf("illegal ICMP packet!\n");
        return -1;
    }
    if(icmph->icmp_type == ICMP_ECHOREPLY && icmph->icmp_id == pid){
        timeval tinterval, trecv, tsend;
        PingPacket * packet = icmp_findpacket(icmph->icmp_seq);

        if(packet == NULL) 
            return -1;

        packet->flag = 0;
        tsend = packet->tv_begin;
        gettimeofday(&trecv, NULL);
        tinterval = icmp_tvsub(trecv, tsend);
        rtt = tinterval.tv_sec * 1000 + tinterval.tv_usec / 1000;

        printf("%d byte from %s: seq=%u ttl=%d rtt=%d ms\n",len,\
                            inet_ntoa(iph->ip_src),icmph->icmp_seq, iph->ip_ttl, rtt);
    } else {
        return -1;
    }
}
const int maxn = 4000;
static sockaddr_in dest;
static char dest_str[maxn];
static char recv_buff[maxn];

void * icmp_send(void * argv){
    gettimeofday(&tv_begin, NULL);
    while(alive){
        int size = 0;
        timeval tv;
        gettimeofday(&tv, NULL);
        //puts("hey jude");
        PingPacket* packet = icmp_findpacket(-1);
        if(packet){
            packet->seq = packet_send;
            packet->flag = 1;
            gettimeofday(&packet->tv_begin, NULL);
        }
        icmp_pack((icmp*)send_buff, packet_send, &tv, 64);
        size = sendto(rawsock, send_buff, 64, 0, (sockaddr*) &dest, sizeof(dest));
        if(size < 0){
            perror("send to error!");
            continue;
        }
        packet_send++;
        sleep(2);
    }
}

static void *icmp_recv(void *argv){
    timeval tv;
    tv.tv_usec = 200;
    tv.tv_sec = 0;

    fd_set readfd;
    while(alive){
        int ret = 0;
        FD_ZERO(&readfd);
        FD_SET(rawsock, &readfd);
        ret = select(rawsock + 1, &readfd, NULL, NULL, &tv);
        //puts("hey jenny");
        switch(ret){
            case -1:
                break;
            case 0:
                break;
            default:
                {
                    int fromLen = 0;
                    sockaddr from;
                    int size = recv(rawsock, recv_buff, sizeof(recv_buff),0);
                    if(errno == EINTR){
                        perror("recvfrom error");
                        continue;
                    }
                    ret = icmp_unpack(recv_buff, size);
                    if(ret == 1){
                        continue;
                    }
                }
                break; //c# standard
        }
       
    }
}
#define K 1024
int main(int argc, const char * argv[]){
    packet_send = 0;
    hostent* host = NULL;
    protoent* protocol = NULL;
    unsigned long inaddr = 1;
    int size = K * 128;
    if(argc < 2){
        printf("IP addr is required!\n");
        return -1;
    }
    protocol = getprotobyname("icmp");
    if(protocol == NULL){
        printf("fail to get protocol");
        return -1;
    }
    memcpy(dest_str,argv[1], strlen(argv[1]) + 1);
    memset(packet, 0, sizeof(packet));

    //puts("rawsock preparing");
    rawsock = socket(AF_INET, SOCK_RAW, protocol->p_proto);
   // puts("rawsock ready");
    if(rawsock < 0){
        perror("Socket");
        return -1;
    }
    pid = getuid();
    
    setsockopt(rawsock, SOL_SOCKET, SO_RCVBUF , &size, sizeof(size));
    bzero(&dest,sizeof(dest));

    dest.sin_family = AF_INET;
    puts("here");
    
    inaddr = inet_addr(argv[1]);
    
    
    if(inaddr == INADDR_NONE){
        host = gethostbyname(argv[1]);
        if(host == NULL){
            perror("gethostbyname");
            return -1;
        }
        memcpy((char*)&dest.sin_addr, host->h_addr, host->h_length);
    } else {
        memcpy((char*)&dest.sin_addr, &inaddr, sizeof(inaddr));
    }

    inaddr = dest.sin_addr.s_addr;
    printf("PING %s (%ld.%ld.%ld.%ld) 56(84) bytes of data.\n",\
            dest_str,(inaddr&0x000000ff)>>0,(inaddr&0x0000ff00)>>8,(inaddr&0x00ff0000)>>16,(inaddr&0xff000000)>>24); 
    alive = 1;
    pthread_t send_id, recv_id;
    int err = 0;
    err = pthread_create(&send_id, NULL, icmp_send, NULL);
    if(err < 0)
        return -1;
    err = pthread_create(&recv_id, NULL, icmp_recv, NULL);
    if(err <0 )
        return -1;

    pthread_join(send_id, NULL);
    pthread_join(recv_id, NULL);
    
    close(rawsock);
    return 0;
}