#include <stdio.h>
#include "net/sock/udp.h"
#include "xtimer.h"

uint8_t buf[128];
int udp_server(int argc, char **argv) {

    sock_udp_ep_t local = SOCK_IPV6_EP_ANY;
    sock_udp_t sock;
    
    if (argc < 2) {
        puts("Please include a port number");
        return 1;
    }

    local.port = atoi(argv[1]);

    /*
    Creates a new UDP sock object. 
    [out]	sock	The resulting sock object.
    [in]	local	Local end point for the sock object. May be NULL. sock_udp_ep_t::netif must either be SOCK_ADDR_ANY_NETIF or equal to sock_udp_ep_t::netif of remote if remote != NULL. If NULL sock_udp_send() may bind implicitly. sock_udp_ep_t::port may also be 0 to bind the sock to an ephemeral port.
    [in]	NULL	Remote end point for the sock object. May be NULL but then the remote parameter of sock_udp_send() may not be NULL or it will always error with return value -ENOTCONN. sock_udp_ep_t::port must not be 0 if remote != NULL. sock_udp_ep_t::netif must either be SOCK_ADDR_ANY_NETIF or equal to sock_udp_ep_t::netif of local if local != NULL.
    [in]	0   	Flags for the sock object. See also sock flags. May be 0.
    */
    if (sock_udp_create(&sock, &local, NULL, 0) < 0) {
        puts("Error creating UDP sock");
        return 1;
    }

    while (1) {
        sock_udp_ep_t remote;
        ssize_t number_of_bytes_recieved;

        /* 
        Receives a UDP message from a remote end point.
        [in]	sock            A UDP sock object.
        [out]	buf             Pointer where the received data should be stored.
        [in]	sizeof(buf)     Maximum space available at data.
        [in]	timeout         Timeout for receive in microseconds. If 0 and no data is available, the function returns immediately. May be SOCK_NO_TIMEOUT for no timeout (wait until data is available).
        [out]	remote          Remote end point of the received data. May be NULL, if it is not required by the application
        */
        if ((number_of_bytes_recieved = sock_udp_recv(&sock, buf, sizeof(buf), SOCK_NO_TIMEOUT,
                                 &remote)) >= 0) {
            puts("Received a message");
        }

        /* Wait with response for one second */
        xtimer_sleep(1);

        /*
        Sends a UDP message to remote end point. 
        [in]	sock                        A UDP sock object. May be NULL. A sensible local end point should be selected by the implementation in that case.
        [in]	buf                         Pointer where the received data should be stored. May be NULL if len == 0.
        [in]	number_of_bytes_recieved	Maximum space available at data.
        [in]	remote                      Remote end point for the sent data. May be NULL, if sock has a remote end point. sock_udp_ep_t::family may be AF_UNSPEC, if local end point of sock provides this information. sock_udp_ep_t::port may not be 0.
        */
        if (sock_udp_send(&sock, buf, number_of_bytes_recieved, &remote) < 0) {
                puts("Error sending reply");
        }     
    }
    /*
    Closes a UDP sock object. 
    */
    sock_udp_close(&sock);
    return 0;
}

int udp_client(int argc, char **argv) {

    sock_udp_ep_t local = SOCK_IPV6_EP_ANY;
    sock_udp_t sock;
    ssize_t number_of_bytes_recieved;

    if (argc != 4) {
        puts("A UDP Message should look like this: udp <server-ip> <port> <message>");
        return 1;
    }

    local.port = atoi(argv[2]);

    if (sock_udp_create(&sock, &local, NULL, 0) < 0) {
        puts("Error creating UDP sock");
        return 1;
    }

    /* Converts an IPv6 address string representation to a byte-represented IPv6 address. */ 
    if (ipv6_addr_from_str((ipv6_addr_t *)&local.addr, argv[1]) == NULL) {
        puts("Error: unable to parse destination address");
        return 1;
    }

    /* Check if addr is a link-local address. */
    if (ipv6_addr_is_link_local((ipv6_addr_t *)&local.addr)) {
        /* choose first interface when address is link local */
        gnrc_netif_t *netif = gnrc_netif_iter(NULL);
        /* netif = stack-specific network interface ID */
        local.netif = (uint16_t)netif->pid;
    }

    size_t send_size = strlen(argv[3]);

    if (send_size > sizeof(buf)) {
        puts("Your message is not displayed correctly because it is too big");
    }

    if (sock_udp_send(&sock, argv[3], send_size, &local) < 0) {
        puts("Error sending message");
        sock_udp_close(&sock);
        return 1;
    }

    if ((number_of_bytes_recieved = sock_udp_recv(&sock, buf, sizeof(buf), SOCK_NO_TIMEOUT,
                        NULL)) < 0) {
        puts("Error receiving message");
    }

    /* Prints the message */
    for (int i = 0; i < number_of_bytes_recieved; i++) {
        printf("%c", buf[i]);
    }
    
    printf("\n");
    
    sock_udp_close(&sock);

    return 0;
}