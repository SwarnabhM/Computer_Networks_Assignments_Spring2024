#include "msocket.h"

int shm_id;
MTPSocketEntry *SM;
SOCK_INFO *sock_info;
sem_t *Sem1;
sem_t *Sem2;
sem_t* SM_mutex;

// Signal handler for SIGINT (Ctrl+C)
void signal_handler(int signum) {
    printf("\nReceived Ctrl+C. Detaching shared memory and quitting.\n");
    // Detach the shared memory segments
    if (shmdt(SM) == -1) {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }
    if (shmdt(sock_info) == -1) {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }
    // Destroy the semaphores
    sem_destroy(Sem1);
    sem_destroy(Sem2);
    sem_destroy(SM_mutex);
    printf("Shared memory and semaphores detached and destroyed successfully.\n");
    exit(EXIT_SUCCESS);
}

// Function to handle incoming messages and send ACKs
void *thread_R() {

    fd_set readfds;
    int max_sd, activity, new_socket;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    Message msg;

    struct timeval timeout;
    timeout.tv_sec = T;
    timeout.tv_usec = 0;
    // Main loop for handling incoming messages

    FD_ZERO(&readfds);
    if (sem_wait(SM_mutex) == -1) {
        perror("sem_wait");
        return NULL;
    }
    max_sd = -1;
    for (int i = 0; i < MAX_MTP_SOCKETS; i++) {
        if(SM[i].socket_alloted){
            FD_SET(SM[i].udp_socket_id, &readfds);
            if (SM[i].udp_socket_id > max_sd) {
                max_sd = SM[i].udp_socket_id;
            }
        }
    }
    if (sem_post(SM_mutex) == -1) {
        perror("sem_post");
        return NULL;
    }
    int count_silence_timeout[MAX_MTP_SOCKETS];
    for(int i=0; i<MAX_MTP_SOCKETS; i++){
        count_silence_timeout[i] = 0;
    }
    while (1) {
        // Clear the socket set
        fd_set temp = readfds;

        // Use select to monitor socket activity
        activity = select(max_sd + 1, &temp, NULL, NULL, &timeout);
        if (activity < 0) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        if(activity == 0){
            timeout.tv_sec = T;
            timeout.tv_usec = 0;
            //ADD THE NOSPACE UPDATE -- done
            FD_ZERO(&readfds);
            if (sem_wait(SM_mutex) == -1) {
                perror("sem_wait");
                return NULL;
            }
            // Add UDP sockets to the set
            max_sd = -1;
            for (int i = 0; i < MAX_MTP_SOCKETS; i++) {
                if(SM[i].socket_alloted){
                    count_silence_timeout[i]++;
                    FD_SET(SM[i].udp_socket_id, &readfds);
                    if (SM[i].udp_socket_id > max_sd) {
                        max_sd = SM[i].udp_socket_id;
                    }
                    if(SM[i].recv_window.nospace==1 && SM[i].recv_window.window_size>0){
                        client_addr = SM[i].destination_addr;
                        Message ack_msg;
                        ack_msg.header.msg_type = 'A';
                        ack_msg.header.seq_no = (SM[i].recv_window.next_seq_no==1)? 15 : (SM[i].recv_window.next_seq_no-1);
                        sprintf(ack_msg.msg, "%d", SM[i].recv_window.window_size);
                        printf("***************\n");
                        printf("sending nospace ack: %d sockfd, %d window size, %d seq no\n", i, SM[i].recv_window.window_size, ack_msg.header.seq_no);
                        printf("***************\n");
                        sendto(SM[i].udp_socket_id, &ack_msg, sizeof(Message), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
                        SM[i].recv_window.nospace = 0;
                    }
                    else if(count_silence_timeout[i]>=MAX_SILENCED_TIMEOUTS){
                        client_addr = SM[i].destination_addr;
                        Message ack_msg;
                        ack_msg.header.msg_type = 'A';
                        ack_msg.header.seq_no = (SM[i].recv_window.next_seq_no==1)? 15 : (SM[i].recv_window.next_seq_no-1);
                        sprintf(ack_msg.msg, "%d", SM[i].recv_window.window_size);
                        printf("***************\n");
                        printf("sending refresh ack: %d sockfd, %d window size, %d seq no\n", i, SM[i].recv_window.window_size, ack_msg.header.seq_no);
                        printf("***************\n");
                        sendto(SM[i].udp_socket_id, &ack_msg, sizeof(Message), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
                        count_silence_timeout[i] = 0;
                    }
                }
            }
            if (sem_post(SM_mutex) == -1) {
                perror("sem_post");
                return NULL;
            }

            continue;
        }

        // Check each UDP socket for incoming messages
        for (int i = 0; i < MAX_MTP_SOCKETS; i++) {
            if(sem_wait(SM_mutex) == -1){
                perror("sem_wait");
                return NULL;
            }
            if (FD_ISSET(SM[i].udp_socket_id, &temp)) {
                
                // Receive message from the UDP socket

                ssize_t recv_len = recvfrom(SM[i].udp_socket_id, &msg, sizeof(Message), 0,
                                            (struct sockaddr *)&client_addr, &addr_len);
                if(SM[i].recv_window.nospace){
                    if(sem_post(SM_mutex)==-1){
                        perror("sem_post");
                        return NULL;
                    }
                    continue;
                }
                if (recv_len < 0) {
                    if(sem_post(SM_mutex)==-1){
                        perror("sem_post");
                        return NULL;
                    }
                    perror("recvfrom");
                    exit(EXIT_FAILURE);
                }
                printf("***************\n");
                printf("received msg on sockfd %d\n", i);
                printf("***************\n");

                if(dropMessage(P)){
                    count_silence_timeout[i]++;
                    printf("***************\n");
                    printf("dropped msg on sockfd %d\n", i);
                    printf("***************\n");

                    if(count_silence_timeout[i]>=MAX_SILENCED_TIMEOUTS){
                        client_addr = SM[i].destination_addr;
                        Message ack_msg;
                        ack_msg.header.msg_type = 'A';
                        ack_msg.header.seq_no = (SM[i].recv_window.next_seq_no==1)? 15 : (SM[i].recv_window.next_seq_no-1);
                        sprintf(ack_msg.msg, "%d", SM[i].recv_window.window_size);
                        printf("***************\n");
                        printf("sending refresh ack: %d sockfd, %d window size, %d seq no\n", i, SM[i].recv_window.window_size, ack_msg.header.seq_no);
                        printf("***************\n");
                        sendto(SM[i].udp_socket_id, &ack_msg, sizeof(Message), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
                        count_silence_timeout[i] = 0;
                    }

                    if(sem_post(SM_mutex)==-1){
                        perror("sem_post");
                        return NULL;
                    }
                    continue;
                }
                else{
                    count_silence_timeout[i] = 0;
                }

                //CHECK IF CORRECT CLIENT IS SENDING -- done
                if(client_addr.sin_addr.s_addr != SM[i].destination_addr.sin_addr.s_addr || client_addr.sin_port != SM[i].destination_addr.sin_port){
                    printf("***************\n");
                    printf("not from correct dest on sockfd %d\n", i);
                    printf("***************\n");
                    if(sem_post(SM_mutex)==-1){
                        perror("sem_post");
                        return NULL;
                    }
                    continue;
                }
                // Process the received message (you need to implement this part)
                // You need to handle ACK messages and data messages separately
                // Update swnd, send ACKs, and store data messages in the receive buffer
                if(msg.header.msg_type=='D'){   
                    if(msg.header.seq_no == SM[i].recv_window.next_seq_no){
                        int idx = SM[i].recv_window.index_to_write;
                        SM[i].recv_window.recv_buff[idx].ack_no = msg.header.seq_no;
                        strcpy(SM[i].recv_window.recv_buff[idx].message, msg.msg);
                        printf("***************\n");
                        printf("data recvd on sockfd %d (as next expected), seq no %d, written on %d idx\n", i, msg.header.seq_no, idx);
                        printf("***************\n");
                        SM[i].recv_window.window_size--;
                        if(SM[i].recv_window.window_size==0)SM[i].recv_window.nospace = 1;
                        printf("***************\n");
                        printf("recv window size of sockfd %d is %d\n", i, SM[i].recv_window.window_size);
                        printf("***************\n");
                        //now find the last in order msg recvd and the next index to write
                        int last_in_order = msg.header.seq_no;
                        int next_idx_to_write = SM[i].recv_window.index_to_write;
                        for(int k=1; k<RECEIVER_MSG_BUFFER; k++){
                            int new_idx = (SM[i].recv_window.index_to_write+k)%RECEIVER_MSG_BUFFER;
                            if(SM[i].recv_window.recv_buff[new_idx].ack_no==-1){
                                break;
                            }
                            int next_exp_seq_no = (last_in_order==15)? 1 : (last_in_order+1);
                            if(SM[i].recv_window.recv_buff[new_idx].ack_no!=next_exp_seq_no)break;
                            last_in_order = next_exp_seq_no;
                            next_idx_to_write = new_idx;
                        }
                        SM[i].recv_window.index_to_write = (next_idx_to_write+1)%RECEIVER_MSG_BUFFER;
                        SM[i].recv_window.next_seq_no = (last_in_order==15)? 1 : (last_in_order+1);
                        printf("***************\n");
                        printf("now, for sockfd %d, index to write: %d, next seq no: %d\n", i, SM[i].recv_window.index_to_write, SM[i].recv_window.next_seq_no);
                        printf("***************\n");
                        Message ack;
                        ack.header.msg_type = 'A';
                        ack.header.seq_no = last_in_order;
                        sprintf(ack.msg, "%d", SM[i].recv_window.window_size);

                        sendto(SM[i].udp_socket_id, &ack, sizeof(Message), 0, (struct sockaddr*)&client_addr, addr_len);
                        printf("***************\n");
                        printf("ack send from sockfd %d, seq no %d, window size %d\n", i, ack.header.seq_no, SM[i].recv_window.window_size);
                        printf("***************\n");

                        if(sem_post(SM_mutex)==-1){
                            perror("sem_post");
                            return NULL;
                        }
                        continue;
                    }
                    int inWindow = 0;
                    int expected_seq_no = SM[i].recv_window.next_seq_no;
                    int new_idx = -1;
                    for(int k=0; k<SM[i].recv_window.window_size; k++){
                        if(msg.header.seq_no==expected_seq_no){
                            new_idx = (SM[i].recv_window.index_to_write + k)%RECEIVER_MSG_BUFFER;
                            inWindow = 1;
                            break;
                        }
                        expected_seq_no = (expected_seq_no+1)%16;
                        if(expected_seq_no==0)expected_seq_no++;
                    }
                    if(inWindow){
                        printf("***************\n");
                        printf("recvd data no sockfd %d, not in order but in window, %d idx, %d seq no\n", i, new_idx, msg.header.seq_no);
                        printf("***************\n");
                        if(SM[i].recv_window.recv_buff[new_idx].ack_no!=msg.header.seq_no){
                            SM[i].recv_window.recv_buff[new_idx].ack_no = msg.header.seq_no;
                            strcpy(SM[i].recv_window.recv_buff[new_idx].message, msg.msg);
                            SM[i].recv_window.window_size--;
                            if(SM[i].recv_window.window_size==0)SM[i].recv_window.nospace = 1;
                            printf("***************\n");
                            printf("sockfd %d, data written at idx %d, seq no %d, window size %d\n", i, new_idx, msg.header.seq_no, SM[i].recv_window.window_size);
                            printf("***************\n");
                        }
                        Message ack;
                        ack.header.msg_type = 'A';
                        ack.header.seq_no = (SM[i].recv_window.next_seq_no==1)? 15 : SM[i].recv_window.next_seq_no-1;
                        sprintf(ack.msg, "%d", SM[i].recv_window.window_size);

                        sendto(SM[i].udp_socket_id, &ack, sizeof(Message), 0, (struct sockaddr*)&client_addr, addr_len);
                        printf("***************\n");
                        printf("ack sent, sockfd %d, seq no %d, window size %d\n", i, ack.header.seq_no, SM[i].recv_window.window_size);
                        printf("***************\n");

                        if(sem_post(SM_mutex)==-1){
                            perror("sem_post");
                            return NULL;
                        }
                        continue;
                    }
                    else{
                        printf("***************\n");
                        printf("recvd data on sockfd %d, not in order and out of window, seq no %d\n", i, msg.header.seq_no);
                        printf("***************\n");

                        Message ack;
                        ack.header.msg_type = 'A';
                        ack.header.seq_no = (SM[i].recv_window.next_seq_no==1)? 15 : SM[i].recv_window.next_seq_no-1;
                        sprintf(ack.msg, "%d", SM[i].recv_window.window_size);

                        sendto(SM[i].udp_socket_id, &ack, sizeof(Message), 0, (struct sockaddr*)&client_addr, addr_len);
                        printf("***************\n");
                        printf("ack sent, sockfd %d, seq no %d, window size %d\n", i, ack.header.seq_no, SM[i].recv_window.window_size);
                        printf("***************\n");
                        
                        if(sem_post(SM_mutex)==-1){
                            perror("sem_post");
                            return NULL;
                        }
                        continue;
                    }
                }
                else if(msg.header.msg_type=='A'){
                    printf("***************\n");
                    printf("ack recvd on sockfd %d, seq no %d, window size %s\n", i, msg.header.seq_no, msg.msg);
                    printf("***************\n");
                    int idx = SM[i].send_window.window_start_index;
                    int ack_in_window = -1;
                    int len = -1;
                    for(int k = 0; k<SM[i].send_window.window_size; k++){
                        int new_idx = (idx+k)%SENDER_MSG_BUFFER;
                        if(SM[i].send_window.send_buff[new_idx].ack_no==-1)break;
                        if(SM[i].send_window.send_buff[new_idx].ack_no==msg.header.seq_no){
                            ack_in_window = new_idx;
                            len = k;
                            break;
                        }
                    }
                    if(ack_in_window!=-1){
                        for(int k = 0; k<=len; k++){
                            int new_idx = (idx+k)%SENDER_MSG_BUFFER;
                            SM[i].send_window.send_buff[new_idx].ack_no = -1;
                        }
                        SM[i].send_window.window_start_index = (ack_in_window+1)%SENDER_MSG_BUFFER;
                        printf("***************\n");
                        printf("sockfd %d swnd window start update to %d\n", i, SM[i].send_window.window_start_index);
                        printf("***************\n");
                    }
                    SM[i].send_window.window_size = atoi(msg.msg);
                    printf("***************\n");
                    printf("sockfd %d swnd window size update %d\n", i, SM[i].send_window.window_size);
                    printf("***************\n");

                    if(sem_post(SM_mutex)==-1){
                        perror("sem_post");
                        return NULL;
                    }
                    continue;
                }
                if(sem_post(SM_mutex)==-1){
                    perror("sem_post");
                    return NULL;
                }
            }else{
                if(SM[i].socket_alloted){
                    count_silence_timeout[i]++;

                    if(count_silence_timeout[i]>=MAX_SILENCED_TIMEOUTS){
                        client_addr = SM[i].destination_addr;
                        Message ack_msg;
                        ack_msg.header.msg_type = 'A';
                        ack_msg.header.seq_no = (SM[i].recv_window.next_seq_no==1)? 15 : (SM[i].recv_window.next_seq_no-1);
                        sprintf(ack_msg.msg, "%d", SM[i].recv_window.window_size);
                        printf("***************\n");
                        printf("sending refresh ack: %d sockfd, %d window size, %d seq no\n", i, SM[i].recv_window.window_size, ack_msg.header.seq_no);
                        printf("***************\n");
                        sendto(SM[i].udp_socket_id, &ack_msg, sizeof(Message), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
                        count_silence_timeout[i] = 0;
                    }
                }

                if(sem_post(SM_mutex)==-1){
                    perror("sem_post");
                    return NULL;
                }
            }
        }
    }


    return NULL;
}

void *thread_S() {
    time_t current_time;
    struct sockaddr_in addr;

    while (1) {
        // Sleep for some time
        usleep((T / 2) * 700000);

        if (sem_wait(SM_mutex) == -1) {
            perror("sem_wait");
            return NULL;
        }

        for (int i = 0; i < MAX_MTP_SOCKETS; i++) {
            if (SM[i].socket_alloted) { // Check if the socket is allocated
                for(int offset=0; offset<SM[i].send_window.window_size; offset++){
                    int idx = (SM[i].send_window.window_start_index + offset)%SENDER_MSG_BUFFER;

                    if(SM[i].send_window.send_buff[idx].ack_no==-1)break;

                    if(SM[i].send_window.send_buff[idx].sent==0){
                        printf("***************\n");
                        printf("sending: %d sockfd, %d idx\n", i, idx);
                        printf("***************\n");
                        addr = SM[i].destination_addr;
                        sendto(SM[i].udp_socket_id, &(SM[i].send_window.send_buff[idx].message), sizeof(Message), 0, (struct sockaddr *)&addr, sizeof(addr));
                        SM[i].send_window.send_buff[idx].time = time(NULL);
                        SM[i].send_window.send_buff[idx].sent = 1;
                        continue;
                    }
                    time(&current_time);
                    double time_gap = difftime(current_time, SM[i].send_window.send_buff[idx].time);
                    if(time_gap >= T){
                        printf("***************\n");
                        printf("sending: %d sockfd, %d idx\n", i, idx);
                        printf("***************\n");  
                        addr = SM[i].destination_addr;
                        sendto(SM[i].udp_socket_id, &(SM[i].send_window.send_buff[idx].message), sizeof(Message), 0, (struct sockaddr *)&addr, sizeof(addr));
                        SM[i].send_window.send_buff[idx].time = time(NULL);
                        continue;
                    }
                }
            }
        }

        if (sem_post(SM_mutex) == -1) {
            perror("sem_post");
            return NULL;
        }
    }

    return NULL;
}


int main() {
    key_t key_SM = ftok("msocket.h", 'M'); // Generate a key for the shared memory segment

    // Create the shared memory segment for SM
    if ((shm_id = shmget(key_SM, MAX_MTP_SOCKETS * sizeof(MTPSocketEntry), IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // Attach the shared memory segment for SM to the process's address space
    SM = (MTPSocketEntry *)shmat(shm_id, NULL, 0);
    if (SM == (MTPSocketEntry *)(-1)) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    key_t key_sockinfo = ftok("msocket.h", 'S');
    // Create the shared memory segment for sock_info
    if ((shm_id = shmget(key_sockinfo, sizeof(SOCK_INFO), IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // Attach the shared memory segment for sock_info to the process's address space
    sock_info = (SOCK_INFO *)shmat(shm_id, NULL, 0);
    if (sock_info == (SOCK_INFO *)(-1)) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    // Create and initialize Sem1
    if ((Sem1 = sem_open("/Sem1", O_CREAT, SEM_PERMS, 0)) == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    // Create and initialize Sem2
    if ((Sem2 = sem_open("/Sem2", O_CREAT, SEM_PERMS, 0)) == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    if ((SM_mutex = sem_open("/SM_mutex", O_CREAT, SEM_PERMS, 1)) == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    // Initialize the shared memory entries for SM
    for (int i = 0; i < MAX_MTP_SOCKETS; i++) {
        SM[i].socket_alloted = 0; // Mark the socket as free
        SM[i].process_id = -1;     // Set process ID to -1 indicating no process has created the socket
        SM[i].udp_socket_id = -1;
        // Initialize other fields as needed
        memset(&(SM[i].destination_addr), 0, sizeof(struct sockaddr_in));
        memset(&(SM[i].send_window), 0, sizeof(swnd));
        memset(&(SM[i].recv_window), 0, sizeof(rwnd));
        SM[i].send_window.last_seq_no = 0;
        SM[i].send_window.window_size = 5;
        SM[i].send_window.window_start_index = 0;
        SM[i].recv_window.index_to_read = 0;
        SM[i].recv_window.next_seq_no = 1;
        SM[i].recv_window.window_size = 5;
        SM[i].recv_window.index_to_write = 0;
        SM[i].recv_window.nospace = 0;
        for(int j=0; j<SENDER_MSG_BUFFER; j++){
            SM[i].send_window.send_buff[j].ack_no = -1;
            SM[i].send_window.send_buff[j].sent = 0;
        }
        for(int j=0; j<RECEIVER_MSG_BUFFER; j++){
            SM[i].recv_window.recv_buff[j].ack_no = -1;
        }
        
    }
    memset(sock_info, 0, sizeof(SOCK_INFO));

    // Register the signal handler for SIGINT
    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        perror("signal");
        exit(EXIT_FAILURE);
    }

    printf("Press Ctrl+C to detach shared memory and quit.\n");
    pthread_t thread_S_tid;
    if (pthread_create(&thread_S_tid, NULL, thread_S, NULL) != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }
    pthread_t thread_R_tid;
    if (pthread_create(&thread_R_tid, NULL, thread_R, NULL) != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    printf("***************\n");
    printf("init done\n");
    printf("***************\n");

    // Wait indefinitely
    while (1) {
        // Wait on Sem1
        // printf("wait started\n");
        if (sem_wait(Sem1) == -1) {
            perror("sem_wait");
            exit(EXIT_FAILURE);
        }
        // printf("out of sem1\n");

        // Process SOCK_INFO
        if (sock_info->sock_id == 0 && sock_info->IP == 0 && sock_info->port == 0) {
            // m_socket call
            sock_info->sock_id = socket(AF_INET, SOCK_DGRAM, 0);
            if (sock_info->sock_id == -1) {
                sock_info->errno_val = errno;
            }
            // Signal Sem2
            if (sem_post(Sem2) == -1) {
                perror("sem_post");
                exit(EXIT_FAILURE);
            }
        } else if (sock_info->sock_id != 0 && sock_info->port != 0) {
            // m_bind call
            printf("in bind\n");
            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_port = sock_info->port;
            addr.sin_addr.s_addr = sock_info->IP;
            int ret = bind(sock_info->sock_id, (struct sockaddr *)&addr, sizeof(addr));
            if (ret == -1) {
                sock_info->errno_val = errno;
                sock_info->sock_id = -1; // Reset sock_id
            }
            printf("bind done %d\n", ret);
            // Signal Sem2
            if (sem_post(Sem2) == -1) {
                perror("sem_post");
                exit(EXIT_FAILURE);
            }
        } else if(sock_info->sock_id!=0 && sock_info->IP==0 && sock_info->port == 0){
            int status = close(sock_info->sock_id);
            if(status==-1){
                sock_info->sock_id = -1;
                sock_info->errno_val = errno;
            }
            // Signal Sem2
            if (sem_post(Sem2) == -1) {
                perror("sem_post");
                exit(EXIT_FAILURE);
            }
        }

    }

    return 0;
}
