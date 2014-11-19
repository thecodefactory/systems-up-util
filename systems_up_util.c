/*
  Written by Neill Miller (neillm@thecodefactory.org)

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  ----------------------
  Compile on Linux with:
  gcc systems_up_util.c -o systems_up_util

  Example run:
  ./systems_up_util -i 192.168.1.1,192.168.1.2 -p 22,80

  This checks the first IP with the first Port, the second IP with the
  second port, etc.  The number of IPs specified must match the number
  of ports specified.
  ----------------------
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

#define MAX_IP_LEN       16
#define MAX_NUM_PORTS    64
#define MAX_NUM_HOSTS    64
#define MAX_COMMAND_LEN 512
#define MAX_MSG_LEN     512

const char *PORT_DELIM = ",";

char msg[MAX_MSG_LEN];

typedef struct
{
    char ip_addrs[MAX_IP_LEN][MAX_NUM_HOSTS];
    int num_ip_addrs;
    int ports[MAX_NUM_PORTS];
    int num_ports;
} host_data_t;

int check_on_service(char *ip_address, int port)
{
    int ret = 0;
    struct sockaddr_in s_addr;
    int s_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s_fd != -1)
    {
        memset(&s_addr, '0', sizeof(s_addr));
        s_addr.sin_family = AF_INET;
        s_addr.sin_port = htons(port);

        if (inet_pton(AF_INET, ip_address, &s_addr.sin_addr) == 1)
        {
            if (connect(s_fd, (struct sockaddr *)&s_addr, sizeof(s_addr)) == 0)
            {
                printf("Connection success, service is active!\n");
                close(s_fd);
            }
            else
            {
                printf("Connection failed (%s)\n", strerror(errno));
                ret = 1;
            }
        }
        else
        {
            printf("Invalid or unsupported network address: %s\n", ip_address);
            ret = 1;
        }
    }
    return ret;
}

int check_host_data_status(host_data_t *hd)
{
    int i = 0;
    int ret = 0;

    time_t t;
    time(&t);

    printf("-------------------------------------------------\n");
    printf("Starting system scan at %s", asctime(localtime(&t)));
    printf("-------------------------------------------------\n");

    for(i = 0; i < hd->num_ports; i++)
    {
        printf("Checking port %d on %s\n", hd->ports[i], hd->ip_addrs[i]);
        ret += check_on_service(hd->ip_addrs[i], hd->ports[i]);
    }
    return ret;
}

int parse_cmdline_args(int argc, char **argv, host_data_t *hd)
{
    int c = 0;
    while((c = getopt (argc, argv, "i:p:")) != -1)
    {
        switch (c)
        {
            case 'i':
            {
                char *cur_host = strtok(optarg, PORT_DELIM);
                do
                {
                    if (cur_host)
                    {
                        snprintf((char *)&(hd->ip_addrs[hd->num_ip_addrs++]), MAX_IP_LEN, "%s", cur_host);
                        //printf("Got Host[%d] = %s\n", hd->num_ip_addrs, cur_host);
                        if (hd->num_ip_addrs > MAX_NUM_HOSTS)
                        {
                            break;
                        }
                    }
                } while(cur_host = strtok(NULL, PORT_DELIM));
                break;
            }
            case 'p':
            {
                char *cur_port = strtok(optarg, PORT_DELIM);
                do
                {
                    if (cur_port)
                    {
                        hd->ports[hd->num_ports++] = atoi(cur_port);
                        //printf("Got Port[%d] = %d\n", hd->num_ports, atoi(cur_port));
                        if (hd->num_ports > MAX_NUM_PORTS)
                        {
                            break;
                        }
                    }
                } while(cur_port = strtok(NULL, PORT_DELIM));
                break;
            }
            default:
                abort ();
        }
    }
    return !(hd->num_ip_addrs && hd->num_ports &&
             (hd->num_ip_addrs == hd->num_ports));
}

int main(int argc, char **argv)
{
    int ret = 0;
    host_data_t hd;
    memset(&hd, 0, sizeof(host_data_t));

    if (parse_cmdline_args(argc, argv, &hd) == 0)
    {
        ret = check_host_data_status(&hd);
    }
    else
    {
        printf("Usage: %s -i IP-ADDRESS1,IP-ADDRESS2,IP-ADDRESSN -p PORT1,PORT2,PORTN\n", argv[0]);
    }
    return ret;
}
