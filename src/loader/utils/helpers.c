#include <loader/utils/helpers.h>

/**
 * Prints help menu.
 * 
 * @return void
 */
void print_help_menu()
{
    printf("Usage: xdpfw [OPTIONS]\n\n");

    printf("  -c, --config         Config file location (default: /etc/xdpfw/xdpfw.conf).\n");
    printf("  -o, --offload        Load the XDP program in hardware/offload mode.\n");
    printf("  -s, --skb            Force the XDP program to load with SKB mode instead of DRV.\n");
    printf("  -t, --time           Duration to run the program (seconds). 0 or unset = infinite.\n");
    printf("  -l, --list           Print config details including filters (exits after execution).\n");
    printf("  -h, --help           Show this help message.\n\n");
    printf("  -v, --verbose        Override config's verbose value.\n");
    printf("      --log-file       Override config's log file value.\n");
    printf("  -i, --interface      Override config's interface value.\n");
    printf("  -u, --update-time    Override config's update time value.\n");
    printf("  -n, --no-stats       Override config's no stats value.\n");
    printf("      --stats-ps       Override config's stats per second value.\n");
    printf("      --stdout-ut      Override config's stdout update time value.\n");
}

/**
 * Handles signals from user.
 * 
 * @param code Signal code.
 * 
 * @return void
 */
void hdl_signal(int code)
{
    cont = 0;
}

/**
 * Parses an IP string with CIDR support. Stores IP in network byte order in ip.ip and CIDR in ip.cidr.
 * 
 * @param ip The IP string.
 * 
 * @return Returns an IP structure with IP and CIDR. 
 */
ip_range_t parse_ip_range(const char *ip)
{
    ip_range_t ret = {0};
    ret.cidr = 32;

    char ip_copy[INET_ADDRSTRLEN + 3];
    strncpy(ip_copy, ip, sizeof(ip_copy) - 1);
    ip_copy[sizeof(ip_copy) - 1] = '\0';

    char *token = strtok(ip_copy, "/");

    if (token)
    {
        ret.ip = inet_addr(token);

        token = strtok(NULL, "/");

        if (token)
        {
            ret.cidr = (u8) strtoul(token, NULL, 10);
        }
    }

    return ret;
}

/**
 * Retrieves protocol name by ID.
 * 
 * @param id The protocol ID
 * 
 * @return The protocol string. 
 */
const char* get_protocol_str_by_id(int id)
{
    switch (id)
    {
        case IPPROTO_TCP:
            return "TCP";

        case IPPROTO_UDP:
            return "UDP";
        
        case IPPROTO_ICMP:
            return "ICMP";
    }

    return "N/A";
}

/**
 * Prints tool name and author.
 * 
 * @return void
 */
void print_tool_info()
{
    printf(
        " __  ______  ____    _____ _                        _ _ \n"
        " \\ \\/ /  _ \\|  _ \\  |  ___(_)_ __ _____      ____ _| | |\n"
        "  \\  /| | | | |_) | | |_  | | '__/ _ \\ \\ /\\ / / _` | | |\n"
        "  /  \\| |_| |  __/  |  _| | | | |  __/\\ V  V / (_| | | |\n"
        " /_/\\_\\____/|_|     |_|   |_|_|  \\___| \\_/\\_/ \\__,_|_|_|\n"
        "\n\n"
    );
}

/**
 * Retrieves nanoseconds since system boot.
 * 
 * @return The current nanoseconds since the system last booted.
 */
u64 get_boot_nano_time()
{
    struct sysinfo sys;
    sysinfo(&sys);

    return sys.uptime * 1e9;
}

/**
 * Parses a port range string and returns the minimum and maximum port.
 * 
 * @param range_str The port range string.
 * 
 * @return The port range as port_range_t type. Fields will be set to 0 on failure.
 */
port_range_t parse_port_range(const char* range_str)
{
    port_range_t ret = {0};

    if (!range_str)
    {
        return ret;
    }

    // Copy range string.
    char range_str_copy[24];
    strncpy(range_str_copy, range_str, sizeof(range_str_copy) - 1);
    range_str_copy[sizeof(range_str_copy) - 1] = '\0';

    // First scan for port ranges with ":".
    char* start = range_str_copy;
    char* end = strchr(range_str_copy, '-');

    if (!end)
    {
        end = strchr(range_str_copy, ':');
    }

    if (end)
    {
        *end = '\0';
        end++;
    }

    char *end_ptr = NULL;

    ret.min = strtol(start, &end_ptr, 10);

    if (end_ptr == start || (*end_ptr != '\0' && !isspace((unsigned char)*end_ptr)))
    {
        return ret;
    }

    if (end)
    {
        ret.max = strtol(end, &end_ptr, 10);

        if (end_ptr == end || (*end_ptr != '\0' && !isspace((unsigned char)*end_ptr)))
        {
            return ret;
        }
    }
    else
    {
        ret.max = ret.min;
    }

    ret.success = 1;

    return ret;
}