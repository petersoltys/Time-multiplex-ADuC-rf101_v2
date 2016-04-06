master_slave_flip = 1;
packet_burst = 920;
COM1='COM3';
COM2='COM4';

% get baudrate from settings.h
[settings_file,errmsg] = fopen('../src/settings.h');
strings = textscan(settings_file, '%s');
idx1 = find(strcmp(strings{1}, 'UART_BAUD_RATE_SLAVE'), 1, 'first');
idx2 = find(strcmp(strings{1}, 'UART_BAUD_RATE_MASTER'), 1, 'first');
slave_baudrate = str2num(strings{1}{idx1+1});
master_baudrate = str2num(strings{1}{idx2+1});
fclose(settings_file);

if master_slave_flip == 1
    slave = serial(COM2,'BaudRate',slave_baudrate);
    COM = COM2;
else
    slave = serial(COM1,'BaudRate',slave_baudrate);
    COM = COM1;
end 

slave2 = serial('COM7','BaudRate',slave_baudrate);
% slave2 = serial('COM7','BaudRate',slave_baudrate);
try
    fopen(slave)
    fopen(slave2)
    disp(COM);
    for j = 1 : 5
        for i = 1 :packet_burst
            fprintf(slave ,'msg0= %.3i ',i);%matlab is automatically adding "\n"
            fprintf(slave2,'msg1= %.3i ',i);%matlab is automatically adding "\n"
        end ;
        pause(1);
    end
    fclose(slave);
    fclose(slave2);

catch
    msg = ['nepodarilo sa otvorit ', COM, ' port.'];
    disp(msg);
    fclose(slave);
    fclose(slave2);
end
