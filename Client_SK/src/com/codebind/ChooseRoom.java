package com.codebind;

import javax.swing.*;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;
import java.util.Objects;

public class ChooseRoom {
    private JPanel availableRooms;
    private JButton connectRoom;
    private JButton createRoom;
    private JList roomList;
    private JButton disconect;
    public JButton refresh;
    private String command;
    private BufferedReader reader;
    private PrintWriter writer;

    public ChooseRoom(JFrame frameServerInfo, Socket clientSocket, String idUser){

        JFrame frame = new JFrame("create or connect to room");
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.setPreferredSize(new Dimension(450,300));
        frame.setResizable(false);

        //add another panel
        frame.add(availableRooms);
        frame.setLocationRelativeTo(null);
        frame.pack();
        frame.setVisible(true);

        //tutaj tworze liste pokoi, sciagam ją z serwera!
        String serverMessage = "";
        System.out.println("Odczyt listy serwerow:");
        try {
            reader = new BufferedReader(new InputStreamReader(clientSocket.getInputStream())); //czeka na znak konca linii
            serverMessage = reader.readLine();
            System.out.println("Lista pokojow: " + serverMessage);
        }
        catch(Exception exc){
            System.out.println("Błąd przy odczytywaniu listy pokoi");
        }
        String[] rooms = serverMessage.split(";");
        DefaultListModel listModel = new DefaultListModel();
        for (String x : rooms){
            listModel.addElement(x);
        }
        roomList.setModel(listModel);

        try {
            writer = new PrintWriter(clientSocket.getOutputStream(), true);
        }
        catch(Exception exc){
            System.out.println("Błąd przy tworzeniu PrinterWriter");
        }

        connectRoom.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                String index = "";
                int temp = roomList.getSelectedIndex();
                //System.out.println(temp);
                if(temp >= 0){  //-1 gdy nic nie wybrane
                    index = roomList.getSelectedValue().toString();
                    //System.out.println(index);
                }
                new ConnectRoom(1, clientSocket, roomList, idUser, index, frame);
            }
        });
        createRoom.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                new ConnectRoom(2, clientSocket, roomList, idUser, "", frame);
            }
        });
        disconect.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                command = "$DISCONNECT";
                try {
                    writer.println(command);
                    System.out.println(command);
                    clientSocket.close();
                    //frame.setEnabled(false); //nie mozna korzystac z tego okna
                    frame.setVisible(false);
                    frameServerInfo.setVisible(true);
                }
                catch(Exception exc){
                    System.out.println("Błąd przy rozłączaniu z serwerem");
                }
            }
        });
        refresh.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                command = "$REFRESH";
                try{
                    writer.println(command);
                    System.out.println(command);
                }
                catch(Exception exc){
                    System.out.println("Bład -> $REFRESH");
                }
                try{
                    String serMessage = reader.readLine();
                    System.out.println("Lista pokojów: " + serMessage);
                    String[] rooms = serMessage.split(";");
                    DefaultListModel listModel = new DefaultListModel();
                    listModel.removeAllElements();
                    for (String x : rooms){
                        listModel.addElement(x);
                    }
                    roomList.setModel(listModel);
                }
                catch(Exception exc){
                    System.out.println("Błąd - odczyt listy pokojów");
                }
            }
        });
    }

}
