#ifndef GAME_H
#define GAME_H

#include<iostream>
#include<unistd.h>
#include"goldchase.h"
#include<memory>
#include<fcntl.h>
#include<sys/mman.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<semaphore.h>
#include<netdb.h>
#include "Map.h"
#include "goldchase.h"
#include <fstream>
#include <string>
#include <signal.h>
#include <mqueue.h>
#include <cstring>
#include <cstdio>
#include <errno.h>
#include <cstdlib>
#include <ctype.h>
#include <vector>

struct goldmine
{
	unsigned short rows;
	unsigned short cols;
	unsigned short gold;
	pid_t daemon_ID;
	pid_t players[5];	
	unsigned char map[0];
};

class Game
{
	private:
		unsigned short cols;
		unsigned short rows;
		unsigned short gold;
		unsigned char current_plr;
		unsigned playernum;
		pid_t currpid;
		pid_t daemon_id;
		unsigned short pos;
		std::string filenames[5];
		unsigned char plrbits[5];
		unsigned char* daemon_map;
		
		std::string mq_name;//set according to the player digit once its assigned
		mqd_t readqueue_fd;
		mqd_t writequeue_fd;
		mqd_t daemon_readqfd_list[5];
		
		bool canwin;
		bool canexit;
		bool mapexists;
		
		Map* M;
		
		int shared_mem;
		goldmine* gmp;
		sem_t* SEM;
		
		int new_sockfd;
			
		static Game* me;
		//Map map;
		
		enum { BUFFSIZE = 1 << 16 };
		
		void make_mapshape(int fd);
		void allocatemem(char* arg);
		
		void get_mem();
		
		void clear(int a);
		
		unsigned char setleastplayer();
		unsigned getnum_players();
		
		void putgold();
		unsigned short putitem(unsigned short c);
		
		void moveleft();
		void moveright();
		void moveup();
		void movedown();
		void pickup();
		void postwin();
		
		bool onleftedge();
		bool onrightedge();
		bool ontopedge();
		bool onbotedge();	
		
		static void reader(int);
		static void cleaner(int);
		static void redrawer(int);
		void read_message();
		void clean_up();
		void write_message(std::string msg,std::string send_name);
		void sigsetup();
		void plyrnameinit();
		void sendsig1();
		
		void create_server_daemon();
		void create_client_daemon(char* IP);
		void daemon_init();
		
		void socket_comm_loop();
		void socket_player_write();
		void socket_player_read(char incoming_byte);
		
		void socket_map_write();
		void socket_map_read();
		
		void socket_txt_write();
		void socket_txt_read(char incoming_byte);
		
		void daemon_sigsetup();
		
		static void player_changer(int);
		static void mapper(int);
		static void msgreader(int);
		
template<typename T>
int READ(int fd,T* obj_ptr,int count)
{
  char* addr=(char*) obj_ptr;
  int offset=0;
  int done;
  while((done=read(fd,addr,count-offset))<count-offset && done!=-1 || done==-1 && errno==EINTR)
  {
    if(done != -1)
    {
      offset+=done;
      addr+=done;
    }
  }
  offset+=done;
  return (done==-1) ? -1 : offset;
}

template<typename T>
int WRITE(int fd,T* obj_ptr,int count)
{
  char* addr=(char*) obj_ptr;
  int offset=0;
  int done;
  while((done=write(fd,addr,count-offset))<count-offset && done!=-1 || done==-1 && errno==EINTR)
  {
    if(done != -1)
    {
      offset+=done;
      addr+=done;
    }
  }
  offset+=done;
  return (done==-1) ? -1 : offset;
}

		
	public:
		Game(char* arg);
		Game();
		~Game();
		
		void gameloop();
		
		

};

#endif
