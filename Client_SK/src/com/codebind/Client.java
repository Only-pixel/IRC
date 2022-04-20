package com.codebind;

import javax.swing.*;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

public class Client {
    private JButton buttonConnect;
    private JPanel panelMain;
    private JButton stworzPokojButton;

    public Client(){

        JFrame frame1 = new JFrame("main window");
        frame1.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame1.setPreferredSize(new Dimension(300,200));

        //add another panel
        frame1.add(panelMain);
        frame1.setLocationRelativeTo(null);
        frame1.pack();
        frame1.setVisible(true);


        buttonConnect.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                //JOptionPane.showMessageDialog(null,"Podaj informacje wymagane aby połączyć się z serwerem");
                frame1.setVisible(false);
                new ServerInfo(frame1);
            }
        });
    }


}
