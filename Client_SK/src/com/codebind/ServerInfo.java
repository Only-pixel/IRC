package com.codebind;

import javax.swing.*;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.*;
import java.net.Socket;

public class ServerInfo {
    private JPanel roomInfo;
    private JTextField server;
    private JTextField port;
    private JTextField idUser;
    private JButton dołączButton;
    private Socket clientSocket1;

    public ServerInfo(JFrame frameClient){
        JFrame frame2 = new JFrame("provide information about server");
        frame2.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame2.setPreferredSize(new Dimension(450,300));

        //add another panel
        frame2.add(roomInfo);
        frame2.setLocationRelativeTo(null);
        frame2.pack();
        frame2.setVisible(true);


        dołączButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                try {
                    //InputStream is = clientSocket.getInputStream();
                    String Message = idUser.getText(); //wyslij serwerowi swoja nazwe
                    if(Message.length()>0) {
                        clientSocket1 = new Socket(server.getText(), Integer.parseInt(port.getText())); //"192.168.0.23", 1234 / localhost nc -vl 1234
                        PrintWriter writer = new PrintWriter(clientSocket1.getOutputStream(), true);
                        writer.println(Message); //napisz wiadomosc
                        frame2.setVisible(false);
                        JOptionPane.showMessageDialog(null, "Polaczono z serwerem!");
                        new ChooseRoom(frame2, clientSocket1, Message);
                    }
                    else{
                        JOptionPane.showMessageDialog(null,"Nie podano nazwy użytkownika");
                    }
                }
                catch(Exception exc){
                    System.out.println("Nie można połączyć się z serwerem");
                    JOptionPane.showMessageDialog(null,"Nie można połączyć się z serwerem");
                }
            }
        });
    }
}
