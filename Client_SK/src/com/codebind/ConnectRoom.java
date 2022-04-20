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

public class ConnectRoom {
    private JButton polacz;
    private JTextField textField1;
    private JPanel windowconnect;
    private String serverMessage;
    private PrintWriter writer;
    private BufferedReader reader;
    private String mode;
    private String owner;
    private String notify;

    public ConnectRoom(int whichMode, Socket clientSocket, JList roomList, String idUser, String whichRoom, JFrame chooseFrame){ //1 - polacz, 2 - stworz

        try {
            this.reader = new BufferedReader(new InputStreamReader(clientSocket.getInputStream())); //czeka na znak konca linii
            this.writer = new PrintWriter(clientSocket.getOutputStream(), true);
        }
        catch(Exception exc){
            System.out.println("Błąd przy tworzeniu PrinterWriter");
        }
        JFrame frame = new JFrame("choose available room");
        frame.setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
        frame.setPreferredSize(new Dimension(450,300));
        frame.setResizable(false);

        //add another panel
        if(whichMode == 2){
            textField1.setText("");
            polacz.setText("Stwórz pokój");
        }
        else{
            textField1.setText(whichRoom);
        }
        frame.add(windowconnect);
        frame.setLocationRelativeTo(null);
        frame.pack();
        frame.setVisible(true);

        polacz.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                String name = textField1.getText();
                try {
                    reader = new BufferedReader(new InputStreamReader(clientSocket.getInputStream())); //czeka na znak konca linii
                    writer = new PrintWriter(clientSocket.getOutputStream(), true);
                }catch (Exception exc) {
                    JOptionPane.showMessageDialog(null, "Wystąpił błąd przy dołączaniu/tworzeniu do pokoju");
                    frame.dispatchEvent(new WindowEvent(frame, WindowEvent.WINDOW_CLOSING));
                }
                if (name.length() > 0) {
                    if (whichMode == 1) { //lacze sie z istniejacym pokojem
                        mode = name;
                        owner= getOwnerId();
                        notify = "Polaczono z wybranym pokojem";
                    } else { //tworze pokoj
                        mode = "$" + name;
                        owner = idUser;
                        notify = "Stworzono pokoj "+name;
                    }
                }
                        try {
                            writer.println(mode); //wysylam nazwe pokoju
                            System.out.println("pokoj do stworzenia: "+name);
                            serverMessage = reader.readLine(); //czy okej
                            System.out.println("serwer message: " + serverMessage);

                            if(serverMessage.length()==0 || serverMessage.equals("\n") || serverMessage.equals("\r") || serverMessage.equals("\r\n")){
                                serverMessage = reader.readLine(); //jesli wysle sie pusty buffer
                                System.out.println("serwer message2: " + serverMessage);
                            } else if(!serverMessage.contains("$")) { //jesli nie polecenie=komenda
                                writer.println("$REFRESH");
                                System.out.println("Refresh");
                                serverMessage = reader.readLine();
                                System.out.println(serverMessage);
                                //writer.println("$"+name);  //wysylam nazwe pokoju, dziala po banie, bez bana juz nie
                                //serverMessage = reader.readLine();
                                //System.out.println("serwer message3: " + serverMessage);
                            }
                            if(serverMessage.contains("$OK_JOIN")) {
                                JOptionPane.showMessageDialog(null, notify);
                                chooseFrame.setVisible(false);
                                new Room(clientSocket, frame, idUser, owner,chooseFrame);
                            }
                        }
                        catch(Exception exc){
                            JOptionPane.showMessageDialog(null,"Wystąpił błąd przy tworzeniu pokoju");
                        }
                    }
        });
    }

    public boolean DoesValueExist(String name, JList roomList) {
        int elements = roomList.getModel().getSize();
        for (int i = 0; i < elements; i++) {
            Object item = roomList.getModel().getElementAt(i).toString();
            System.out.println("nazwa pokoju: "+name);
            System.out.println("jaki pokoj mam: "+item);
            if(item.equals(name)){
                return true;
            }
        }
        return false;
    }

    public String getOwnerId(){
        String user = "";
        //none if i do not know the owner = user is not the owner
        return user;
    }

}
