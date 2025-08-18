/*
Serial-MIDI to ALSA-MIDI conversion script for use on Rpi4
*/
#include &lt;stdio.h&gt;
#include &lt;stdlib.h&gt;
#include &lt;string.h&gt;
#include &lt;unistd.h&gt;
#include &lt;fcntl.h&gt;
#include &lt;errno.h&gt;
#include &lt;termios.h&gt;
#include &lt;alsa/asoundlib.h&gt;
#include &lt;alsa/seq_midi_event.h&gt;

#define SERIAL_PORT "/dev/serial0"
#define BAUD_RATE B38400
#define MIDI_PORT "hw:24,0,0"

int serial_fd;
snd_seq_t *seq_handle;
int out_port;
snd_midi_event_t *midi_event_parser;
size_t message_length = 0;

// Function to handle errors and clean up resources
void error_exit(const char *message) {
    perror(message);
    if (serial_fd &gt;= 0) close(serial_fd);
    if (seq_handle) snd_seq_close(seq_handle);
    if (midi_event_parser) snd_midi_event_free(midi_event_parser);
    exit(EXIT_FAILURE);
}

// Function to send a MIDI message using snd_midi_event_encode
void send_midi_message(unsigned char *message, size_t length) {
    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);
    snd_seq_ev_set_source(&ev, out_port);
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);

    // Encode the MIDI bytes into an ALSA sequencer event
    if (snd_midi_event_encode(midi_event_parser, message, length, &ev) &lt; 0) {
        fprintf(stderr, "Error encoding MIDI message\n");
        return;
    }

    snd_seq_event_output_direct(seq_handle, &ev);
    snd_seq_drain_output(seq_handle);
}

// Function to determine the length of a MIDI message based on the status byte
size_t get_message_length(unsigned char status_byte) {
    if ((status_byte & 0xF0) == 0xC0 || (status_byte & 0xF0) == 0xD0) {
        return 2;  // Program Change and Channel Pressure messages are 2 bytes long
    } else if ((status_byte & 0xF0) == 0xF0 && status_byte != 0xF0 && status_byte != 0xF7) {
        return 1;  // System Real-Time messages are 1 byte long
    } else {
        return 3;  // Most other MIDI messages are 3 bytes long
    }
}

int main() {
    configure_serial_port();
    printf("Listening to %s ...\n", SERIAL_PORT);
    configure_sequencer();
    printf("Opened ALSA sequencer\n");
    unsigned char buffer[3];
    size_t buffer_index = 0;

    while (1) {
        unsigned char byte;
        ssize_t bytes_read = read(serial_fd, &byte, 1);
        if (bytes_read &gt; 0) {
            if (byte & 0x80) {  // If it's a status byte
                buffer_index = 0; // Reset buffer index to 0 to start a new message
                buffer[buffer_index++] = byte;
                message_length = get_message_length(byte);
            } else {           // Not a status byte
                buffer[buffer_index++] = byte;
            }

            if (buffer_index &gt;= message_length) {
                send_midi_message(buffer, buffer_index);
                // Reset to 1 to keep the status byte for running-status, will be set to 0 if the next read is a status byte
                buffer_index =  1;  
            }
        } else if (bytes_read &lt; 0) {
            fprintf(stderr, "Error reading from serial port: %s\n", strerror(errno));
            // Optionally, can add a delay here to avoid busy-waiting in case of repeated errors
            usleep(10000);  // Sleep for 10 milliseconds
        }
    }

    close(serial_fd);
    snd_seq_close(seq_handle);
    snd_midi_event_free(midi_event_parser);
    return 0;
}
