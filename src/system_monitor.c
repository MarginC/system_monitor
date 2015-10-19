// © 2014 Abimael Martell
// System Monitor - multi-platform system monitor
// See LICENSE

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include "system_monitor.h"

static int daemon (int nochdir, int noclose)
{
    pid_t pid;

    pid = fork ();
    /* In case of fork is error. */
    if (pid < 0)
        exit(-1);
    /* In case of this is parent process. */
    else if (pid != 0)
        exit(0);

    /* Become session leader and get pid. */
    if (setsid() == -1)
        exit(-1);

    /* Change directory to root. */
    if (! nochdir)
        chdir ("/");

    /* File descriptor close. */
    if (! noclose)
    {
        int fd = open ("/dev/null", O_RDWR, 0);
        if (fd != -1)
        {
            dup2 (fd, STDIN_FILENO);
            dup2 (fd, STDOUT_FILENO);
            dup2 (fd, STDERR_FILENO);
            if (fd > 2)
                close (fd);
        }
    }

    umask (0027);

    return 0;
}

sigar_t *sigar;
struct global_options globalOptions;

void log_line (char *line, int log_level)
{
    time_t current_time;
    struct tm *p;
    char time_str[80], tmpBuf[MAX_BUFFER_SIZE + 80];

    current_time = time(NULL);
    p = localtime(&current_time);
    strftime(time_str, sizeof(time_str), "%D %r", p);

    if (log_level == LOG_INFO) {
        snprintf(tmpBuf, sizeof(tmpBuf), LOG_INFO_LINE, time_str, line);
    }

    puts(tmpBuf);
}

void parse_arguments (int argc, char **argv)
{
    int opt, index;
    char *out;

    globalOptions.port = DEFAULT_HTTP_PORT;

    // for http cache
    globalOptions.start_time = time(NULL);
    gmtime(&globalOptions.start_time);

    static struct option long_options[] = {
        {"daemon", no_argument, 0, 'd'},
        {"port", required_argument, 0, 'p'},
        {0, 0, 0, 0}
    };

    while (1) {
        opt = getopt_long(argc, argv, "dp:", long_options, &index);
        if(opt == -1) break;

        switch (opt) {
            case 'd':
                if (daemon(0, 0)) {
                    printf("daemon failed:%s\n", strerror(errno));
                    exit(1);
                }
            break;
            case 'p':
                strtol(optarg, &out, 10);
                if (*out) {
                    printf("\"%s\" is not a valid port number\n", optarg);
                    exit(1);
                } else {
                    globalOptions.port = optarg;
                }
            break;
            default:
                exit(1);
        }
    }
}

json_object * get_stats_json (void)
{
    sigar_t *sigar;
    sigar_mem_t memory;
    sigar_cpu_list_t cpu_list;
    sigar_cpu_info_list_t cpu_info_list;
    sigar_file_system_list_t file_system_list;
    sigar_file_system_usage_t file_system_usage;
    sigar_net_interface_list_t net_interface_list;
    sigar_net_interface_config_t net_interface_config, net_interface_primary;
    sigar_net_interface_stat_t net_interface_stat;
    sigar_uptime_t uptime;
    sigar_proc_list_t proc_list;
    sigar_proc_state_t proc_state;
    sigar_proc_cpu_t proc_cpu;
    sigar_proc_cred_name_t proc_cred;
    sigar_proc_mem_t proc_mem;
    sigar_thread_cpu_t proc_threads;
    sigar_net_info_t net_info;
    sigar_sys_info_t sys_info;
    sigar_loadavg_t load_average;
    sigar_proc_args_t proc_args;
    sigar_proc_exe_t proc_exe;
    json_object *stats_json, *memory_json, *cpu_json, *cores_json, *core_json,
                *file_system_json, *file_systems_json, *file_system_usage_json,
                *net_interfaces_json, *net_interface_json, *net_interface_address_json,
                *net_interface_stat_json, *proc_list_json, *proc_json, *proc_cpu_json,
                *proc_mem_json, *net_info_json, *sys_info_json, *load_average_json,
                *proc_args_json, *proc_exe_json;
    char *state_string;
    int primary_interface;
    unsigned int i, p;

    sigar_open(&sigar);
    sigar_mem_get(sigar, &memory);

    stats_json = json_object_new_object();

    /* get ram usage */
    memory_json = json_object_new_object();
    json_object_object_add(memory_json, "total", json_object_new_int64(memory.ram));
    json_object_object_add(memory_json, "free", json_object_new_int64(memory.actual_free));
    json_object_object_add(memory_json, "used", json_object_new_int64(memory.actual_used));
    json_object_object_add(memory_json, "used_percent", json_object_new_int64(memory.used_percent));
    json_object_object_add(memory_json, "free_percent", json_object_new_int64(memory.free_percent));
    json_object_object_add(stats_json, "memory", memory_json);

    /* get cpu's info */
    sigar_cpu_info_list_get(sigar, &cpu_info_list);
    cpu_json = json_object_new_object();
    json_object_object_add(cpu_json, "vendor", json_object_new_string(cpu_info_list.data[0].vendor));
    json_object_object_add(cpu_json, "model", json_object_new_string(cpu_info_list.data[0].model));
    json_object_object_add(cpu_json, "mhz", json_object_new_int(cpu_info_list.data[0].mhz));
    json_object_object_add(cpu_json, "total_cores", json_object_new_int(cpu_info_list.data[0].total_cores));
    sigar_cpu_info_list_destroy(sigar, &cpu_info_list);

    sigar_cpu_list_get(sigar, &cpu_list);
    cores_json = json_object_new_array();
    for (i = 0; i < cpu_list.number; i++) {
        core_json = json_object_new_object();
        json_object_object_add(core_json, "user",     json_object_new_int64(cpu_list.data[i].user));
        json_object_object_add(core_json, "sys",      json_object_new_int64(cpu_list.data[i].sys));
        json_object_object_add(core_json, "nice",     json_object_new_int64(cpu_list.data[i].nice));
        json_object_object_add(core_json, "idle",     json_object_new_int64(cpu_list.data[i].idle));
        json_object_object_add(core_json, "wait",     json_object_new_int64(cpu_list.data[i].wait));
        json_object_object_add(core_json, "irq",      json_object_new_int64(cpu_list.data[i].irq));
        json_object_object_add(core_json, "soft_irq", json_object_new_int64(cpu_list.data[i].soft_irq));
        json_object_object_add(core_json, "stolen",   json_object_new_int64(cpu_list.data[i].stolen));
        json_object_object_add(core_json, "total",    json_object_new_int64(cpu_list.data[i].total));
        json_object_array_add(cores_json, core_json);
    }

    json_object_object_add(cpu_json, "cores", cores_json);
    json_object_object_add(stats_json, "cpu", cpu_json);
    sigar_cpu_list_destroy(sigar, &cpu_list);

    /* get file systems info and usage */
    sigar_file_system_list_get(sigar, &file_system_list);
    file_systems_json = json_object_new_array();
    for (i = 0; i < file_system_list.number; i++) {
        file_system_json = json_object_new_object();
        json_object_object_add(file_system_json, "dir", json_object_new_string(file_system_list.data[i].dir_name));
        json_object_object_add(file_system_json, "dev", json_object_new_string(file_system_list.data[i].dev_name));
        json_object_object_add(file_system_json, "type", json_object_new_string(file_system_list.data[i].type_name));
        json_object_object_add(file_system_json, "sys_type", json_object_new_string(file_system_list.data[i].sys_type_name));
        json_object_object_add(file_system_json, "options", json_object_new_string(file_system_list.data[i].options));

        file_system_usage_json = json_object_new_object();
        sigar_file_system_usage_get(sigar, file_system_list.data[i].dir_name, &file_system_usage);
        json_object_object_add(file_system_usage_json, "use_percent", json_object_new_double(file_system_usage.use_percent));
        json_object_object_add(file_system_usage_json, "total", json_object_new_int((int) file_system_usage.total));
        json_object_object_add(file_system_usage_json, "free", json_object_new_int64(file_system_usage.free));
        json_object_object_add(file_system_usage_json, "used", json_object_new_int64(file_system_usage.used));
        json_object_object_add(file_system_usage_json, "available", json_object_new_int64(file_system_usage.avail));
        json_object_object_add(file_system_usage_json, "files", json_object_new_int64(file_system_usage.files));
        json_object_object_add(file_system_json, "usage", file_system_usage_json);

        json_object_array_add(file_systems_json, file_system_json);
    }
    json_object_object_add(stats_json, "file_systems", file_systems_json);
    sigar_file_system_list_destroy(sigar, &file_system_list);

    /* get network interfaces info */
    sigar_net_interface_config_primary_get(sigar, &net_interface_primary);

    sigar_net_interface_list_get(sigar, &net_interface_list);

    net_interfaces_json = json_object_new_array();

    for (i = 0; i < net_interface_list.number; i++) {
        primary_interface = 0;
        sigar_net_interface_config_get(sigar, net_interface_list.data[i], &net_interface_config);

        if (strcmp(net_interface_config.name, net_interface_primary.name) == 0) {
            primary_interface = 1;
        }

        net_interface_json = json_object_new_object();

        json_object_object_add(net_interface_json, "name", json_object_new_string(net_interface_config.name));
        json_object_object_add(net_interface_json, "type", json_object_new_string(net_interface_config.type));
        json_object_object_add(net_interface_json, "primary", json_object_new_boolean(primary_interface));


        net_interface_address_json = json_object_new_object();
        json_object_object_add(net_interface_address_json, "ip", json_object_new_string(inet_to_string(net_interface_config.address.addr.in)));
        json_object_object_add(net_interface_address_json, "mac", json_object_new_string(mac_address_to_string(net_interface_config.hwaddr.addr.mac)));
        json_object_object_add(net_interface_json, "address", net_interface_address_json);

        net_interface_stat_json = json_object_new_object();
        sigar_net_interface_stat_get(sigar, net_interface_config.name, &net_interface_stat);
        json_object_object_add(net_interface_stat_json, "speed", json_object_new_int64(net_interface_stat.speed));
        json_object_object_add(net_interface_stat_json, "rx_packets", json_object_new_int64(net_interface_stat.rx_packets));
        json_object_object_add(net_interface_stat_json, "tx_packets", json_object_new_int64(net_interface_stat.tx_packets));
        json_object_object_add(net_interface_stat_json, "rx_bytes", json_object_new_int64(net_interface_stat.rx_bytes));
        json_object_object_add(net_interface_stat_json, "tx_bytes", json_object_new_int64(net_interface_stat.tx_bytes));
        json_object_object_add(net_interface_json, "stat", net_interface_stat_json);

        json_object_array_add(net_interfaces_json, net_interface_json);
    }

    json_object_object_add(stats_json, "network_interfaces", net_interfaces_json);

    /* uptime */
    sigar_uptime_get(sigar, &uptime);
    json_object_object_add(stats_json, "uptime", json_object_new_double(uptime.uptime));

    /* processes list */
    sigar_proc_list_get(sigar, &proc_list);
    proc_list_json = json_object_new_array();
    for (i = 0; i < proc_list.number; i++) {
        int pid = proc_list.data[i];

        sigar_proc_state_get(sigar, pid, &proc_state);
        sigar_proc_cpu_get(sigar, pid, &proc_cpu);
        sigar_proc_cred_name_get(sigar, pid, &proc_cred);
        sigar_proc_mem_get(sigar, pid, &proc_mem);
        sigar_thread_cpu_get(sigar, pid, &proc_threads);

        proc_json = json_object_new_object();

        switch (proc_state.state) {
            case SIGAR_PROC_STATE_SLEEP:
                state_string = "Sleeping";
                break;
            case SIGAR_PROC_STATE_RUN:
                state_string = "Running";
                break;
            case SIGAR_PROC_STATE_STOP:
                state_string = "Stopped";
                break;
            case SIGAR_PROC_STATE_ZOMBIE:
                state_string = "Zombie";
                break;
            case SIGAR_PROC_STATE_IDLE:
                state_string = "Idle";
                break;
            default:
                state_string = "";
                break;
        }

        json_object_object_add(proc_json, "name",    json_object_new_string(proc_state.name));
        json_object_object_add(proc_json, "pid",     json_object_new_int64(pid));
        json_object_object_add(proc_json, "state",   json_object_new_string(state_string));
        json_object_object_add(proc_json, "user",    json_object_new_string(proc_cred.user));
        json_object_object_add(proc_json, "group",   json_object_new_string(proc_cred.group));
        json_object_object_add(proc_json, "threads", json_object_new_int(proc_state.threads));

        proc_cpu_json = json_object_new_object();
        json_object_object_add(proc_cpu_json, "start_time", json_object_new_int64(proc_cpu.start_time));
        json_object_object_add(proc_cpu_json, "user", json_object_new_int64(proc_cpu.user));
        json_object_object_add(proc_cpu_json, "sys", json_object_new_int64(proc_cpu.sys));
        json_object_object_add(proc_cpu_json, "total", json_object_new_int64(proc_cpu.total));
        json_object_object_add(proc_cpu_json, "last_time", json_object_new_int64(proc_cpu.last_time));
        json_object_object_add(proc_cpu_json, "percent", json_object_new_double(proc_cpu.percent));
        json_object_object_add(proc_json,     "cpu",  proc_cpu_json);

        proc_mem_json = json_object_new_object();
        json_object_object_add(proc_mem_json, "size", json_object_new_int64(proc_mem.size));
        json_object_object_add(proc_mem_json, "resident", json_object_new_int64(proc_mem.resident));
        json_object_object_add(proc_mem_json, "share", json_object_new_int64(proc_mem.share));
        json_object_object_add(proc_mem_json, "minor_faults", json_object_new_int64(proc_mem.minor_faults));
        json_object_object_add(proc_mem_json, "major_faults", json_object_new_int64(proc_mem.major_faults));
        json_object_object_add(proc_mem_json, "page_faults", json_object_new_double(proc_mem.page_faults));
        json_object_object_add(proc_json,     "memory",  proc_mem_json);

        proc_args_json = json_object_new_array();
        sigar_proc_args_get(sigar, pid, &proc_args);

        for (p = 0; p < proc_args.number; p++) {
            json_object_array_add(proc_args_json, json_object_new_string(proc_args.data[p]));
        }

        json_object_object_add(proc_json, "arguments", proc_args_json);
        sigar_proc_args_destroy(sigar, &proc_args);

        sigar_proc_exe_get(sigar, pid, &proc_exe);
        proc_exe_json = json_object_new_object();
        json_object_object_add(proc_exe_json, "name", json_object_new_string(proc_exe.name));
        json_object_object_add(proc_exe_json, "cwd", json_object_new_string(proc_exe.cwd));
        json_object_object_add(proc_exe_json, "root", json_object_new_string(proc_exe.root));

        json_object_object_add(proc_json, "exe", proc_exe_json);

        json_object_array_add(proc_list_json, proc_json);
    }
    json_object_object_add(stats_json, "processes", proc_list_json);
    sigar_proc_list_destroy(sigar, &proc_list);

    /* network info */
    net_info_json = json_object_new_object();
    sigar_net_info_get(sigar, &net_info);
    json_object_object_add(net_info_json, "host_name", json_object_new_string(net_info.host_name));
    json_object_object_add(net_info_json, "default_gateway", json_object_new_string(net_info.default_gateway));
    json_object_object_add(net_info_json, "default_gateway_interface", json_object_new_string(net_info.default_gateway_interface));
    json_object_object_add(net_info_json, "primary_dns", json_object_new_string(net_info.primary_dns));
    json_object_object_add(stats_json,    "network_info", net_info_json);

    /* system info */
    sys_info_json = json_object_new_object();
    sigar_sys_info_get(sigar, &sys_info);
    json_object_object_add(sys_info_json, "name", json_object_new_string(sys_info.name));
    json_object_object_add(sys_info_json, "version", json_object_new_string(sys_info.version));
    json_object_object_add(sys_info_json, "arch", json_object_new_string(sys_info.arch));
    json_object_object_add(sys_info_json, "machine", json_object_new_string(sys_info.machine));
    json_object_object_add(sys_info_json, "description", json_object_new_string(sys_info.description));
    json_object_object_add(sys_info_json, "patch_level", json_object_new_string(sys_info.patch_level));
    json_object_object_add(sys_info_json, "vendor", json_object_new_string(sys_info.vendor));
    json_object_object_add(sys_info_json, "vendor_version", json_object_new_string(sys_info.vendor_version));
    json_object_object_add(sys_info_json, "vendor_name", json_object_new_string(sys_info.vendor_name));
    json_object_object_add(sys_info_json, "vendor_code_name", json_object_new_string(sys_info.vendor_code_name));
    json_object_object_add(stats_json,    "system_info", sys_info_json);


    /* load average */
    sigar_loadavg_get(sigar, &load_average);
    load_average_json = json_object_new_array();

    for (i = 0; i < 3; i++) {
        json_object_array_add(load_average_json, json_object_new_double(load_average.loadavg[i]));
    }

    json_object_object_add(stats_json, "load_average", load_average_json);

    sigar_close(sigar);

    return stats_json;
}
