package com.codebind;

import javax.swing.*;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.WindowEvent;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;

public class Room {
    private JPanel Room;
    private JList uzytkownicy;
    private JTextField textField1;
    private JButton send;
    private JTextArea messages;
    private JButton ban;
    private JTextField toBan;
    private JPanel banpan;
    private JPanel banpan2;
    private JButton disconnect;
    public Socket clientSocketr;
    private String idUser;
    private String command;
    BufferedReader reader;
    JFrame chooseFrame;
    Thread thread;
    JFrame frame;
    RoomB objB;
    RoomA objA;

    private class RoomA {
        public RoomA() {
            send.addActionListener(new ActionListener() {
                @Override
                public void actionPerformed(ActionEvent e) {
                    //wyslij wiadomosc na serwer
                    if (textField1.getText().length() > 0) {
                        try {
                            PrintWriter writer = new PrintWriter(clientSocketr.getOutputStream(), true);
                            writer.println(textField1.getText()); //napisz wiadomosc
                            messages.append(idUser + ": " + textField1.getText() + "\n"); //ten klient
                            textField1.setText("");
                        } catch (Exception exc) {
                            JOptionPane.showMessageDialog(null, "Błąd przy wysyłaniu wiadomości na serwer!");
                        }

                    }
                }
            });
            ban.addActionListener(new ActionListener() {
                @Override
                public void actionPerformed(ActionEvent e) {
                    if (toBan.getText().length() > 0) {
                        try {
                            PrintWriter writer = new PrintWriter(clientSocketr.getOutputStream(), true);
                            writer.println("$REM " + toBan.getText()); //napisz wiadomosc
                            toBan.setText("");
                        } catch (Exception exc) {
                            JOptionPane.showMessageDialog(null, "Błąd przy wysyłaniu wiadomości na serwer!");
                        }

                    }
                }
            });
            disconnect.addActionListener(new ActionListener() {
                @Override
                public void actionPerformed(ActionEvent e) {
                    try {
                        PrintWriter writer = new PrintWriter(clientSocketr.getOutputStream(), true);
                        command = "$LEAVE";
                        writer.println(command);
                        System.out.println(command);
                        thread.stop(); //zabij watek
                        chooseFrame.setVisible(true);
                        frame.setVisible(false);
                        frame.dispatchEvent(new WindowEvent(frame, WindowEvent.WINDOW_CLOSING));
                    } catch (Exception exc) {
                        JOptionPane.showMessageDialog(null, "Opuszczanie pokoju");
                    }
                }
            });
        }
    }

    private class RoomB implements Runnable{
        public RoomB(){

        }

        public void run() {
            //System.out.println("nowy watek");
            //watek
            try {
                Boolean check = true;
                //BufferedReader reader = new BufferedReader(new InputStreamReader(clientSocketr.getInputStream())); //czeka na znak konca linii
                //noinspection InfiniteLoopStatement
                while (check) {
                    String serverMessage = reader.readLine();
                    if(serverMessage.contains("$ROOM") || serverMessage.contains("$FAIL")){
                        chooseFrame.setVisible(true);
                        frame.setVisible(false);
                        frame.dispatchEvent(new WindowEvent(frame, WindowEvent.WINDOW_CLOSING));
                        check = false;
                    }
                    messages.append(serverMessage + "\n");
                    System.out.println("run: "+serverMessage);
                    serverMessage = "";
                }
                System.out.println("thread stop");
                //String Message = reader.readLine(); //odbieram na sztywno liste pokojow po banie
                //return;
            } catch (Exception exc) {
                System.out.println("Błąd przy odczytywaniu wiadomosci z serwera");
            }

        }
    }
    public Room(Socket clientSocket, JFrame frameConnect, String idUser, String owner, JFrame chooseFrame) {
        clientSocketr = clientSocket;
        this.idUser = idUser;
        frame = new JFrame("Witam w pokoju");
        frame.setDefaultCloseOperation(JFrame.DO_NOTHING_ON_CLOSE);
        frame.setPreferredSize(new Dimension(550,400));
        frame.setResizable(false);

        this.chooseFrame = chooseFrame;
        //System.out.println(getUserId().equals(owner));
        if(!idUser.equals(owner)){
            JFrame frame2 = new JFrame();
            frame2.add(banpan);
            frame2.add(banpan2);
            frame2.setVisible(false);
        }

        frame.add(Room);
        frame.setLocationRelativeTo(null);
        frame.pack();
        frame.setVisible(true);
        frameConnect.setVisible(false);
        messages.setLineWrap(true);

        try {
            reader = new BufferedReader(new InputStreamReader(clientSocketr.getInputStream()));
        }
        catch (Exception exc) {
            JOptionPane.showMessageDialog(null, "Tworzenie obiektu");
        }
        objB = new RoomB();
        thread = new Thread(objB);
        thread.start();
        objA = new RoomA();

    }

}
