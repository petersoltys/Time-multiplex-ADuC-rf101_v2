master_slave_flip = 1;
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
    master = serial(COM1,'BaudRate',master_baudrate);
    COM = COM1;
else
    master = serial(COM2,'BaudRate',master_baudrate);
    COM = COM2;
end 


try
    fopen(master)
    disp(COM);
    disp('synchronize');
    fprintf(master,'SYNC$');
    fclose(master);
catch
    msg = ['nepodarilo sa otvorit ', COM, ' port.'];
    disp(msg);
    fclose(master);
end
