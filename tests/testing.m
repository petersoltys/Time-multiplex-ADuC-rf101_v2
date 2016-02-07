master_slave_flip = 1;
packet_burst = 10;
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
    slave = serial(COM2,'BaudRate',slave_baudrate);
else
    master = serial(COM2,'BaudRate',master_baudrate);
    slave = serial(COM1,'BaudRate',slave_baudrate);
end 
 
% j = batch('pararel_test','AttachedFiles',{'Master.m','Slave.m'})
% 
% wait(j);
% load(j);



try
    fopen(slave);
    fopen(master);
    msg=['master ',COM1];
    disp(msg);
    msg=['slave ',COM2];
    disp(msg);
    
    for j = 1 : 5
        for i = 1 :packet_burst
            fprintf(slave,'%d$',i+1000);
        end ;
        pause(0.5);
    end
    
%      for j = 1 : 5
        for i = 1 :packet_burst
            str = fscanf(master, '%d')
        end ;
%      end
    fclose(slave);
    fclose(master);
    
catch
    msg = ['nepodarilo sa otvorit ', COM1,' alebo ', COM2 ' port.'];
    disp(msg);
    fclose(slave);
    fclose(master);
end
