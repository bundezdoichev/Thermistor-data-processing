import serial
import sys
import threading


# switchable flag for reading data
read_flag = 1


class Input_Reading_Thread(threading.Thread):
    def __init__(self, user_input: list):
        super(Input_Reading_Thread, self).__init__(target=get_user_input, args=(user_input,))
        self.user_input = user_input

    def readable(self):
        if len(self.user_input) == 0:
            return False
        else:
            return True

    def wait_value(self):
        while True:
            if self.readable():
                return self.user_input.pop(0)


def data_process(ser: serial.Serial, thread: Input_Reading_Thread, destination_file):

    with open(destination_file, 'a') as file:
        while True:
            if thread.readable():
                cmd = thread.user_input.pop(0)
                cmd_process(ser, thread, cmd)
            data = ser.readline().decode()
            file.write(data)


def cmd_process(ser: serial.Serial, thread: Input_Reading_Thread, cmd):
    global read_flag
    
    if cmd == '1':
        written = ser.write(cmd.encode())
        print(written)
        if read_flag:
            read_flag = 0
            print("reading data is paused")
        else:
            read_flag = 1
            print("reading data is resumed")

    elif cmd == '2':
        ser.write(cmd.encode())
        print("Enter the new value for delay or press 'n' to get back")
        
        delay = thread.wait_value()
        if delay.isdecimal():
            ser.write(delay.encode())
            print(f"The delay has been changed to {delay} seconds.")
        else:
            print("The value should be a decimal. Press 'n' to left the delay unchanged or enter the proper value.")
            while True:
                # print("cmd_process")
                delay = input()
                if delay.isdecimal():
                    ser.write(delay.encode())
                    print(f"The delay has been changed to {delay} seconds.")
                    break
                else:
                    print("The value should be a decimal. Press 'n' to left the delay unchanged or enter the proper value.")

    else:
        print(f"Found command '{cmd}' which wasn't expected.")
        print_help()


def print_help():
    print("""Press '1' key to pause/resume reading data.
Press '2' key to set pause between reading data. Default is 1 second.""")


def get_user_input(input_list: list):
    while True:
        # print("get_user_input")
        input_list.append(
                input()
        )


def init_input_reading_thread():
    input_list = []
    input_reading_thread = Input_Reading_Thread(input_list)
    input_reading_thread.daemon = True
    input_reading_thread.start()

    return input_reading_thread


def main():
    # get CLI arguments
    port_name = sys.argv[1]
    destination_file = sys.argv[2]

    # connect to the serial port
    try:
        ser = serial.Serial(port_name, 115200)
        print_help()
    except serial.SerialException:
        print(f'The device {port_name} can not be found')
        exit(-1)

    # initialize the thread and make it started
    input_reading_thread = init_input_reading_thread()

    # make MCU continue reading the data from periferal if
    # read_flag was set to '0' before exiting.
    try:
        # read data from port and write to the file
        data_process(ser, input_reading_thread, destination_file)
    except KeyboardInterrupt:
        if read_flag == 0:
            ser.write(b'1')
        exit(0)


if __name__ == "__main__":
    main()

