master_slave_flip = 0;
packet_burst = 20;

if master_slave_flip == 1
    slave = serial('COM3','BaudRate',128000);
else
    slave = serial('COM4','BaudRate',128000);
end 

% if master_slave_flip == 1
%     slave = serial('COM3','BaudRate',9600);
% else
%     slave = serial('COM5','BaudRate',9600);
% end 


try
    fopen(slave)
    for j = 1 : 5
        for i = 1 :packet_burst
            fprintf(slave,' *message %d $',i);
        end ;
        pause(0.5);
    end
    fclose(slave);

catch
    fclose(slave);
end
