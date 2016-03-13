master_slave_flip = 1;
packet_burst = 330;
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


try
    fopen(slave)
    disp(COM);
%     for j = 1 : 5
        for i = 1 :packet_burst
            fprintf(slave,'msg = %.3i',i);%matlab is automatically adding "\n"
        end ;
%         pause(0.5);
%     end
    fclose(slave);

catch
    msg = ['nepodarilo sa otvorit ', COM, ' port.'];
    disp(msg);
    fclose(slave);
end
