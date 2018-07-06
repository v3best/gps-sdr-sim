#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>
#include <yunsdr_api.h>

#define NOTUSED(V) ((void) V)
#define MHZ(x) ((long long)(x*1000000.0 + .5))
#define GHZ(x) ((long long)(x*1000000000.0 + .5))
#define NUM_SAMPLES 2600
#define BUFFER_SIZE (NUM_SAMPLES * 2 * sizeof(int16_t))


struct stream_cfg {
    long long bw_hz; // Analog banwidth in Hz
    long long fs_hz; // Baseband sample rate in Hz
    long long lo_hz; // Local oscillator frequency in Hz
    const char* rfport; // Port name
    double gain_db; // Hardware gain
};

static void usage() {
    fprintf(stderr, "Usage: yunsdrplayer [options]\n"
        "  -t <filename>      Transmit data from file (required)\n"
        "  -a <attenuation>   Set TX attenuation [dB] (default -20.0)\n"
        "  -b <bw>            Set RF bandwidth [MHz] (default 5.0)\n"
        "  -n <network>       YunSDR network IP (default 192.168.1.10)\n");
    return;
}

static bool stop = false;

static void handle_sig(int sig)
{
    NOTUSED(sig);
    stop = true;
}

static char* readable_fs(double size, char *buf, size_t buf_size) {
    int i = 0;
    const char* units[] = {"B", "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
    while (size > 1024) {
        size /= 1024;
        i++;
    }
    snprintf(buf, buf_size, "%.*f %s", i, size, units[i]);
    return buf;
}

/*
 * 
 */
int main(int argc, char** argv) {
    char buf[1024];
    int opt;
    const char* path = NULL;
    struct stream_cfg txcfg;
    FILE *fp = NULL;
    const char *ip = NULL;
    YUNSDR_DESCRIPTOR *yunsdr;
    
    // TX stream default config
    txcfg.bw_hz = MHZ(3.0); // 3.0 MHz RF bandwidth
    txcfg.fs_hz = MHZ(2.6); // 2.6 MS/s TX sample rate
    txcfg.lo_hz = GHZ(1.575420); // 1.57542 GHz RF frequency
    txcfg.rfport = "A";
    txcfg.gain_db = -20.0;
    
    while ((opt = getopt(argc, argv, "t:a:b:n:")) != EOF) {
        switch (opt) {
            case 't':
                path = optarg;
                break;
            case 'a':
                txcfg.gain_db = atof(optarg);
                if(txcfg.gain_db > 0.0) txcfg.gain_db = 0.0;
                if(txcfg.gain_db < -80.0) txcfg.gain_db = -80.0;
                break;
            case 'b':
                txcfg.bw_hz = MHZ(atof(optarg));
                if(txcfg.bw_hz > MHZ(5.0)) txcfg.bw_hz = MHZ(5.0);
                if(txcfg.bw_hz < MHZ(1.0)) txcfg.bw_hz = MHZ(1.0);
                break;
            case 'n':
                ip = optarg;
                break;
            default:
                printf("Unknown argument '-%c %s'\n", opt, optarg);
                usage();
                return EXIT_FAILURE;
        }
    }
  
    signal(SIGINT, handle_sig);
    
    if( path == NULL ) {
        printf("Specify a path to a file to transmit\n");
        usage();
        return EXIT_FAILURE;
    }
    
    fp = fopen(path, "rb");
    if (fp==NULL) {
        fprintf(stderr, "ERROR: Failed to open TX file: %s\n", path);
        return EXIT_FAILURE;
    }
    fseek(fp, 0L, SEEK_END);
    size_t sz = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    readable_fs((double)sz, buf, sizeof(buf));
    printf("* Transmit file size: %s\n", buf);
    
    printf("* Acquiring devices\n");
    if(ip == NULL)
        ip = "192.168.1.10";
    yunsdr = yunsdr_open_device(ip);
    if(yunsdr == NULL) {
        fprintf(stderr, "Open [%s] failed!\n", ip);
        fclose(fp);
        return EXIT_FAILURE;
    }

    printf("* Open device success!\n");

    yunsdr_set_ref_clock (yunsdr, INTERNAL_REFERENCE);
    yunsdr_set_vco_select (yunsdr, AUXDAC1);
    yunsdr_set_auxdac1 (yunsdr, 1450);
    yunsdr_set_trx_select (yunsdr, TX);
    yunsdr_set_duplex_select (yunsdr, FDD);

    yunsdr_set_tx_lo_freq (yunsdr, txcfg.lo_hz);
    yunsdr_set_tx_sampling_freq (yunsdr, txcfg.fs_hz);
    yunsdr_set_tx_rf_bandwidth (yunsdr, txcfg.bw_hz);
    yunsdr_set_tx_attenuation (yunsdr, TX1_CHANNEL, txcfg.gain_db * -1000);

    printf("* Creating TX buffer\n");

    short *tx_buf = malloc(BUFFER_SIZE);
    if (tx_buf == NULL) {
        fprintf(stderr, "Cannot alloc %d Bytes memory for yunsdr failed\n", NUM_SAMPLES);
        fclose(fp);
        yunsdr_close_device(yunsdr);
        return EXIT_FAILURE;
    }
    yunsdr_enable_tx(yunsdr, NUM_SAMPLES, START_TX_NORMAL);
    printf("* Transmit starts...\n");    
    // Keep writing samples while there is more data to send and no failures have occurred.
    while (!feof(fp) && !stop) {
        fread(tx_buf, sizeof(short), BUFFER_SIZE / sizeof(short), fp);
        int nwrite = yunsdr_write_samples(yunsdr, (void **)&tx_buf, BUFFER_SIZE, TX1_CHANNEL, 0);
        if (nwrite < 0) {
            fprintf(stderr, "Write data error, will be closed ...\n");
            break;
        }
    }
    printf("Done.\n");

    fclose(fp);
    free(tx_buf);
    yunsdr_close_device(yunsdr);

    return EXIT_SUCCESS;
}

