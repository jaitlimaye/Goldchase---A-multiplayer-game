#include "Game.h"

Game* Game::me;

Game::Game(char* arg)
{
	me = this;
	canwin= false;
	canexit= false;
	mapexists = false;
	plyrnameinit();
	daemon_id = 0;
	
	this->SEM = sem_open("/gold",O_CREAT|O_EXCL,S_IRUSR|S_IWUSR,1);
	if(SEM == SEM_FAILED)
	{
		if(errno == EEXIST)
		{
			std::cerr<<"Game is already running. Do not provide a map file"<<std::endl;
			exit(1);
		}
		else perror("Create and open semaphore");
		exit(1);
	}
	if(isdigit(arg[0]))
	{
		//ipaddress
		sem_wait(SEM); //WHERE TO POST????
		if(fork()==0)
		{
			create_client_daemon(arg);
		}
		sem_wait(SEM);
		sem_post(SEM);
		this->shared_mem= shm_open("/goldmemory",O_RDWR,S_IRUSR|S_IWUSR);
		if(shared_mem == -1)
		{
			perror("Open Shared Memory");
			sem_post(SEM);
			sem_close(SEM);
			exit(1);
		}
		sem_wait(SEM);
		gmp =(goldmine*)mmap(nullptr,sizeof(goldmine)+(rows*cols),PROT_READ|PROT_WRITE,MAP_SHARED,shared_mem,0);
		if(gmp == MAP_FAILED)
		{
			perror("Mmap goldmine");
			sem_post(SEM);
			sem_close(SEM);
			close(shared_mem);
			exit(1);
		}
		sem_post(SEM);
		cols = gmp->cols;
		rows = gmp->rows;
		gold = gmp->gold;
		daemon_id = gmp->daemon_ID;
		currpid = getpid();

		current_plr = setleastplayer();
		pos = putitem(current_plr);
	 
		sigsetup();
		kill(daemon_id,SIGHUP);
		//usleep(1000);
		kill(daemon_id,SIGUSR1);
	}
	else
	{
		int filedesc = open(arg,O_RDONLY);
		if(filedesc == -1)
		{
			std::cerr<<"Couldn't file named "<<arg<<std::endl;
			clear(1);
		}
		else
		{
			this->current_plr = G_PLR0;
			this->currpid = getpid();
			this->playernum = 0;
		
			sem_wait(SEM);
			make_mapshape(filedesc);
			
			allocatemem(arg);
			putgold();
			pos = putitem(current_plr);
			sem_post(SEM);
			if(fork()==0)
			{
				create_server_daemon();
			}
			sigsetup();
			daemon_id = gmp->daemon_ID;
		}
	}

}

Game::Game()
{
	me = this;
	canwin= false;
	canexit= false;
	mapexists = false;
	plyrnameinit();
	
	daemon_id = 0;
	this->SEM = sem_open("/gold",0);
	if(SEM == SEM_FAILED)
	{
		if(errno == ENOENT)
		{
			std::cerr<<"No one is playing!Provide a map file"<<std::endl;
			sem_post(SEM);
			exit(1);
		}
		else perror("Open Semaphore");
		sem_post(SEM);
		exit(1);
	}
	this->shared_mem= shm_open("/goldmemory",O_RDWR,S_IRUSR|S_IWUSR);
	if(shared_mem == -1)
	{
		perror("Open Shared Memory");
		sem_post(SEM);
		sem_close(SEM);
		exit(1);
	}
	sem_wait(SEM);
	gmp =(goldmine*)mmap(nullptr,sizeof(goldmine)+(rows*cols),PROT_READ|PROT_WRITE,MAP_SHARED,shared_mem,0);
	if(gmp == MAP_FAILED)
	{
		perror("Mmap goldmine");
		sem_post(SEM);
		sem_close(SEM);
		close(shared_mem);
		exit(1);
	}
	sem_post(SEM);
	cols = gmp->cols;
	rows = gmp->rows;
	gold = gmp->gold;
	currpid = getpid();
	daemon_id = gmp->daemon_ID;
	current_plr = setleastplayer();
	pos = putitem(current_plr);
	kill(daemon_id,SIGHUP);
	kill(daemon_id,SIGUSR1);
	sendsig1(); 
	sigsetup();
}

void Game::get_mem()
{
	
	
	this->shared_mem= shm_open("/goldmemory",O_RDWR,0);
	if(shared_mem == -1)
	{
		perror("Open Shared Memory");
		sem_post(SEM);
		sem_close(SEM);
		exit(1);
	}
	sem_wait(SEM);
	gmp =(goldmine*)mmap(nullptr,sizeof(goldmine)+(rows*cols),PROT_READ|PROT_WRITE,MAP_SHARED,shared_mem,0);
	if(gmp == MAP_FAILED)
	{
		perror("Mmap goldmine");
		sem_post(SEM);
		sem_close(SEM);
		close(shared_mem);
		exit(1);
	}

	daemon_map = new unsigned char [cols*rows];
	
	cols = gmp->cols;
	rows = gmp->rows;
	for(unsigned i =0; i< cols*rows; i++)
	{
		daemon_map[i] = gmp->map[i];
	}
	gold = gmp->gold;
	currpid = getpid();
		sem_post(SEM);
}

unsigned Game::getnum_players()
{
	unsigned count = 0;
	for(unsigned i = 0;i<5; i++)
	{
		if(gmp->players[i] != 0)
		{
			count++;
		}
	}
	return count;
}

unsigned char Game::setleastplayer()
{

	if(gmp->players[0] == 0)
	{
		gmp->players[0] = currpid;
		playernum = 0;
		return G_PLR0;
	}
	else if(gmp->players[1] == 0)
	{
		gmp->players[1] = currpid;
		playernum = 1;
		return G_PLR1;
	}
	else if(gmp->players[2] == 0)
	{
		gmp->players[2] = currpid;
		playernum = 2;
		return G_PLR2;
	}
	else if(gmp->players[3] == 0)
	{
		gmp->players[3] = currpid;
		playernum = 3;
		return G_PLR3;
	}
	else if(gmp->players[4] == 0)
	{
		gmp->players[4] = currpid;
		playernum = 4;
		return G_PLR4;
	}
	else 
	{
		std::cerr<<"Already have 5 players"<<std::endl;
		sem_close(SEM);
		close(shared_mem);
		exit(1);
	}
}

void Game::make_mapshape(int fd)
{
	
	unsigned char buffer[BUFFSIZE];
	ssize_t bytes_read = 0;
	int k = 0;
	int colscount =0;
	int cols = 0;
	int rows = 0;
	
	
	while((bytes_read = read(fd,buffer,sizeof(buffer)))>0)
	{

		this->gold = (unsigned short)(buffer[k]) - 48;
		
		for(k = 2;k<bytes_read;k++)
		{
			colscount++;
			if(buffer[k] == '\n')
			{
				rows++;
				if(colscount >cols)
				{
					cols = colscount;
				}
				colscount = 0;			
			}
		}
	}
	this->rows = rows;
	this->cols = cols -1;
	close(fd);
	
}

void Game::allocatemem(char* arg)
{
	std::fstream f;
	f.open(arg,std::ios::in);
	
	if(!f)
	{
		std::cerr<<"File not opened"<<std::endl;
		sem_post(SEM);
		clear(1);
	}
	
	this->shared_mem = shm_open("/goldmemory",O_CREAT|O_EXCL|O_RDWR,S_IRUSR|S_IWUSR);
	if(shared_mem == -1)
	{
		perror("Create shared Memory");
		sem_post(SEM);
		clear(1);
		
	}
	int ft = ftruncate(shared_mem,sizeof(goldmine)+(rows*cols));
	if(ft == -1)
	{
		perror("Ftruncate");
		sem_post(SEM);
		clear(1);
	}
	
		
	gmp =(goldmine*)mmap(nullptr,sizeof(goldmine)+(rows*cols),PROT_READ|PROT_WRITE,MAP_SHARED,shared_mem,0);
	if(gmp == MAP_FAILED)
	{
		perror("Mmap goldmine");
		sem_post(SEM);
		clear(1);
	}
	gmp->rows = this->rows;
	gmp->cols = this->cols;	
	gmp->players[0] = this->currpid;
	for(unsigned i = 1; i<5; i++)
	{
		gmp->players[i] = 0;
	}
	gmp->gold = this->gold;
	
	std::string buf;
	std::getline(f,buf);
	for(int i = 0; i<this->rows; i++)
	{
		std::getline(f,buf);
		for(int j = 0; j<this->cols; j++)
		{
			if(buf[j] == '*')
			{
				gmp->map[(i*this->cols)+j] = G_WALL;
			}
			else if(buf[j] == ' ')
			{
			gmp->map[(i*this->cols)+j] = 0;
			}
			else
			{
				std::cerr<<"Invalid character in file"<<std::endl;
				sem_post(SEM);
				clear(1);
			}
		}
	}

	f.close();

}


void Game::clear(int a)
{
	daemon_id = gmp->daemon_ID;
	sem_close(SEM);
	close(shared_mem);
	
	mq_close(readqueue_fd);
  	mq_unlink(mq_name.c_str());
  	if(mapexists) 
  	{
  		gmp->map[pos] &= ~current_plr;
		gmp->players[playernum] = 0;	
		sendsig1();
	  	delete M;
	}
	//gmp->players[playernum] = 0;
	kill(daemon_id,SIGHUP);
	/*
	if(getnum_players() == 0)
	{
		sem_unlink("/gold");
		shm_unlink("/goldmemory");
	}*/
	if(a == 1)
	{
		exit(1);
	}
}

void Game::putgold()
{
	unsigned short g = this->gold;
	if(g>0)
	{
	putitem(G_GOLD);
	g--;
	for(int i = 0;i < g;i++)
	{
		putitem(G_FOOL);
	}
	}	
}
void Game::gameloop()
{

	try
	{
		M = new Map((unsigned char *)gmp->map,gmp->rows,gmp->cols);
	}catch(const std::exception& e)
	{
		std::cerr<<e.what()<<std::endl;
		sem_close(SEM);
		close(shared_mem);
		gmp->map[pos] &= ~current_plr;
		gmp->players[playernum] = 0;
		if(getnum_players() == 0)
		{
			sem_unlink("/gold");
			shm_unlink("/goldmemory");
		}
		exit(1);
	}
	mapexists = true;
	int key = 0;
	
	while(!canexit)
	{
		M->drawMap();
		pickup();
		key = M->getKey();
		if(key == 104)
		{
			moveleft();
			sendsig1();
		}
		else if(key == 106)
		{
			movedown();
			sendsig1();
		}
		else if(key == 107)
		{
			moveup();
			sendsig1();
		}
		else if(key == 108)
		{
			moveright();
			sendsig1();
		}
		else if(key == 109)
		{
			//getplayer returns bit and then getmessage using that bit
			unsigned char mask =0;
			
			for(int i =0;i<5;i++)
			{
				pid_t ID = gmp->players[i];
				if(ID != 0 && ID != currpid)
				{
					mask |= plrbits[i];
				}
			}
			
			unsigned char selectedplr = M->getPlayer(mask);
			for(int i =0;i<5;i++)
			{
				if(plrbits[i] == selectedplr)
				{
					write_message("Player " + std::to_string(playernum + 1) +" says: " + M->getMessage(),filenames[i]);
				}
			}
		}
		else if(key == 98)
		{
			std::string msg = M->getMessage();
			for(int i =0;i<5;i++)
			{
				pid_t ID = gmp->players[i];
				if(ID != 0 && ID != currpid)
				{
					write_message("Player " + std::to_string(playernum + 1) +" says: " + msg,filenames[i]);
				}
			}
			
		}
		else if(key == 81)
		{
			gmp->map[pos] &= ~current_plr;
			sendsig1();
			canexit = true;
		}
	}
	daemon_id = gmp->daemon_ID;
	sem_close(SEM);
	close(shared_mem);
	gmp->map[pos] &= ~current_plr;
	gmp->players[playernum] = 0;
	
	kill(daemon_id,SIGHUP);
	mq_close(readqueue_fd);
  	mq_unlink(mq_name.c_str());
  	/*
	if(getnum_players() == 0)
	{
		sem_unlink("/gold");
		shm_unlink("/goldmemory");
	}*/
	return;
}


void Game::moveleft()
{
	if(!onleftedge() && gmp->map[pos-1] != G_WALL )
	{
		sem_wait(SEM);
		gmp->map[pos] &= ~current_plr;
		gmp->map[pos-1] |= current_plr;
		pos = pos -1;
		sem_post(SEM);
	}
	else if(onrightedge() & canwin)
	{
		sem_wait(SEM);
		gmp->map[pos] &= ~current_plr;
		sem_post(SEM);
		M->drawMap();
		postwin();
		canexit = true;
	}
}

void Game::moveright()
{
	
	if(!onrightedge()&& gmp->map[pos+1] != G_WALL)
	{
		sem_wait(SEM);
		gmp->map[pos] &= ~current_plr;
		gmp->map[pos+1] |= current_plr;
		pos = pos +1;
		sem_post(SEM);
	}
	else if(onrightedge() & canwin)
	{
		sem_wait(SEM);
		gmp->map[pos] &= ~current_plr;
		sem_post(SEM);
		M->drawMap();
		postwin();
		canexit = true;
	}
}

void Game::moveup()
{
	if(!ontopedge()&& gmp->map[pos-cols] != G_WALL)
	{
		sem_wait(SEM);
		gmp->map[pos] &= ~current_plr;
		gmp->map[pos-cols] |= current_plr;
		pos = pos-cols;
		sem_post(SEM);
	}
	else if(ontopedge() & canwin)
	{
		sem_wait(SEM);
		gmp->map[pos] &= ~current_plr;
		sem_post(SEM);
		M->drawMap();
		postwin();
		canexit = true;
	}
}

void Game::movedown()
{
	if(!onbotedge()&& gmp->map[pos+cols] != G_WALL)
	{
		sem_wait(SEM);
		gmp->map[pos] &= ~current_plr;
		gmp->map[pos+cols] |= current_plr;
		pos = pos+cols;
		sem_post(SEM);
	}
	else if(onbotedge() & canwin)
	{
		sem_wait(SEM);
		gmp->map[pos] &= ~current_plr;
		sem_post(SEM);
		M->drawMap();
		postwin();
		canexit = true;
	}
}

void Game::pickup()
{
	if( (gmp->map[pos] & G_GOLD) == G_GOLD)
	{
		M->postNotice("YOU GOT THE GOLD!! GET TO THE EDGE QUICK!!");
		gmp->map[pos] &= ~G_GOLD;
		canwin = true;
	}
	else if((gmp->map[pos] & G_FOOL) == G_FOOL)
	{
		M->postNotice("You got the gold! just kidding it's FOOLS GOLD!");
		gmp->map[pos] &= ~G_FOOL ;
	}
}

void Game::postwin()
{
	for(int i =0;i<5;i++)
	{
		pid_t ID = gmp->players[i];
		if(ID != 0 && ID != currpid)
		{
			write_message("Player " + std::to_string(playernum + 1) +" Wins!",filenames[i]);
		}
	}
	M->postNotice("YOU WIN!!");
}
bool Game::onleftedge()
{
		return (pos%cols == 0);
}

bool Game::onrightedge()
{
		return (pos%cols == cols - 1);
}

bool Game::ontopedge()
{
	return(pos < cols);	
}

bool Game::onbotedge()
{
	return(pos >= ((rows*cols)-cols));
}

unsigned short Game::putitem(unsigned short c)
{
	unsigned short i;
	unsigned char t;
	do
	{
	i = std::rand()%(this->rows * this->cols);

	}while(gmp->map[i] != 0);
	gmp->map[i] = c;
	return i;
}



Game::~Game()
{
	delete M;
}

void Game::reader(int)
{
	me->read_message();
}

void Game::cleaner(int)
{
	me->clear(1);
}

void Game::redrawer(int)
{
	me->M->drawMap();
}

void Game::read_message()
{
	struct sigevent mq_notification_event;
  	mq_notification_event.sigev_notify=SIGEV_SIGNAL;
  	mq_notification_event.sigev_signo=SIGUSR2;
  	mq_notify(Game::readqueue_fd, &mq_notification_event); //readqueue_fd DEFINE

  	int err;
  	char msg[121];
  	memset(msg, 0, 121);
  	
  	while((err=mq_receive(Game::readqueue_fd, msg, 120, nullptr))!=-1)
  	{
    	Game::M->postNotice(msg);
    	memset(msg, 0, 121);//set all characters to '\0'
  	}
  
  	if(errno!=EAGAIN)
  	{
    	perror("mq_receive");
    	exit(1);
  	}
}

void Game::write_message(std::string msg,std::string send_name)
{

  	if((writequeue_fd=mq_open(send_name.c_str(), O_WRONLY|O_NONBLOCK))==-1) //mq_name;
  	{
    	perror("mq_open");
    	exit(1);
  	}
  	
  	char message_text[121];
  	memset(message_text, 0, 121);
  	strncpy(message_text, msg.c_str(), 120);
  	if(mq_send(writequeue_fd, message_text, strlen(message_text), 0)==-1)
  	{
    	perror("mq_send");
    	exit(1);
  	}
  	mq_close(writequeue_fd);
}

void Game::sigsetup()
{
	struct sigaction exit_handler;
  	exit_handler.sa_handler=cleaner;
  	sigemptyset(&exit_handler.sa_mask);
  	exit_handler.sa_flags=0;
  	sigaction(SIGINT, &exit_handler, nullptr);
	
	struct sigaction update;
  	update.sa_handler=redrawer; 
  	sigemptyset(&update.sa_mask); 
  	update.sa_flags=0;
  	sigaction(SIGUSR1, &update, nullptr); 
	
  	struct sigaction messaging;
  	messaging.sa_handler=reader; 
  	sigemptyset(&messaging.sa_mask); 
  	messaging.sa_flags=0;
  	sigaction(SIGUSR2, &messaging, nullptr); 

  	struct mq_attr mq_attributes;
  	mq_attributes.mq_flags=0;
  	mq_attributes.mq_maxmsg=10;
  	mq_attributes.mq_msgsize=120;

	for(int i =0;i<5;i++)
	{
		pid_t ID = gmp->players[i];
		if(ID == currpid)
		{
			mq_name = filenames[i];
		}
	}
	
  	if((readqueue_fd=mq_open(mq_name.c_str(), O_RDONLY|O_CREAT|O_EXCL|O_NONBLOCK,S_IRUSR|S_IWUSR, &mq_attributes))==-1)
  	{
    	perror("mq_open");
    	exit(1);
  	}
  
  	struct sigevent mq_notification_event;
  	mq_notification_event.sigev_notify=SIGEV_SIGNAL;
  	mq_notification_event.sigev_signo=SIGUSR2;
  	mq_notify(readqueue_fd, &mq_notification_event);
}

void Game::sendsig1()
{
	for(int i =0;i<5;i++)
	{
		pid_t ID = gmp->players[i];
		if(ID != 0 && ID != currpid)
		{
			kill(ID,SIGUSR1);
		}
	}
}

void Game::plyrnameinit()
{
	filenames[0] = "/G_PLR0_mq";
	filenames[1] = "/G_PLR1_mq";
	filenames[2] = "/G_PLR2_mq";
	filenames[3] = "/G_PLR3_mq";
	filenames[4] = "/G_PLR4_mq";
	
	plrbits[0] = G_PLR0;
	plrbits[1] = G_PLR1;
	plrbits[2] = G_PLR2;
	plrbits[3] = G_PLR3;
	plrbits[4] = G_PLR4;
}


void Game::create_server_daemon()
{	
	daemon_init();
	daemon_sigsetup();
	this->SEM = sem_open("/gold",0);
	if(SEM == SEM_FAILED)
	{
		if(errno == ENOENT)
		{
			std::cerr<<"No one is playing!Provide a map file"<<std::endl;
			sem_post(SEM);
			exit(1);
		}
		else perror("Open Semaphore");
		sem_post(SEM);
		exit(1);
	}
	get_mem();
	daemon_id = currpid;
	
	
	
	for(unsigned i = 1; i<5; i++)
	{
		gmp->players[i] = 0;
	}
	sem_wait(SEM);
	gmp->daemon_ID = currpid;
	sem_post(SEM);
	
	int sockfd; //file descriptor for the socket
  	int status; //for error checking


  	const char* portno="42424"; 
  	struct addrinfo hints;
  	memset(&hints, 0, sizeof(hints)); //zero out everything in structure
  	hints.ai_family = AF_UNSPEC; //don't care. Either IPv4 or IPv6
  	hints.ai_socktype=SOCK_STREAM; // TCP stream sockets
  	hints.ai_flags=AI_PASSIVE; //fill in the IP of the server for me

  	struct addrinfo *servinfo;
  	if((status=getaddrinfo(NULL, portno, &hints, &servinfo))==-1)
  	{
    	fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    	exit(1);
  	}
  	sockfd=socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

  	/*avoid "Address already in use" error*/
  	int yes=1;
  	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, 4)==-1)
  	{
    	perror("setsockopt");
    	exit(1);
  	}

  //We need to "bind" the socket to the port number so that the kernel
  //can match an incoming packet on a port to the proper process
  	if((status=bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen))==-1)
  	{
    	perror("bind");
    	exit(1);
  	}
  	//when done, release dynamically allocated memory
  	freeaddrinfo(servinfo);

  	if(listen(sockfd,1)==-1)
  	{
    	perror("listen");
    	exit(1);
  	}
  	struct sockaddr_in client_addr;
  	socklen_t clientSize=sizeof(client_addr);
  	do
  	{
  		new_sockfd=accept(sockfd, (struct sockaddr*) &client_addr, &clientSize);
  	}
 	while(new_sockfd == -1 && errno == EINTR);
	
	//short unsigned int* msg = &rows;
  	WRITE(new_sockfd,&rows,sizeof(rows));
  	//msg = &cols;
  	WRITE(new_sockfd,&cols,sizeof(cols));
  	for(int i = 0; i<rows*cols; i++)
  	{
  		
  		unsigned char mp = daemon_map[i];
  		WRITE(new_sockfd,&mp,sizeof(mp) );
  	}
  	socket_player_write();
  	socket_comm_loop();
}

void Game::create_client_daemon(char* IP)
{
	
	daemon_init();
  	int status; //for error checking
	daemon_sigsetup();
  //change this # between 2000-65k before using
  	const char* portno="42424"; 

  	struct addrinfo hints;
  	memset(&hints, 0, sizeof(hints)); //zero out everything in structure
  	hints.ai_family = AF_UNSPEC; //don't care. Either IPv4 or IPv6
  	hints.ai_socktype=SOCK_STREAM; // TCP stream sockets

  	struct addrinfo *servinfo;
  //address of server
  	if((status=getaddrinfo(IP, portno, &hints, &servinfo))==-1)
  	{
    	fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    	exit(1);
  	}
  	new_sockfd=socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

  	if((status=connect(new_sockfd, servinfo->ai_addr, servinfo->ai_addrlen))==-1)
  	{
    	perror("connect");
    	exit(1);
  	}
  //release the information allocated by getaddrinfo()
  	freeaddrinfo(servinfo);
  	unsigned short int* intbuf;
  	READ(new_sockfd,&rows,sizeof(rows));
  	
  	READ(new_sockfd,&cols,sizeof(cols));
  	
  	daemon_map = new unsigned char [cols*rows];
  	
	unsigned char mp ;
	currpid = getpid();
  	for(int i = 0; i<rows*cols; i++)
  	{
  		READ(new_sockfd,&mp,sizeof(mp) );
  		daemon_map[i] = mp;
  	}
  	currpid = getpid();
  	this->shared_mem = shm_open("/goldmemory",O_CREAT|O_EXCL|O_RDWR,S_IRUSR|S_IWUSR);
	if(shared_mem == -1)
	{
		perror("Open Shared Memory");
		sem_post(SEM);
		sem_close(SEM);
		exit(1);
	}
  	int ft = ftruncate(shared_mem,sizeof(goldmine)+(rows*cols));
	if(ft == -1)
	{
		perror("Ftruncate");
		sem_post(SEM);
		clear(1);
	}
	
		
	gmp =(goldmine*)mmap(nullptr,sizeof(goldmine)+(rows*cols),PROT_READ|PROT_WRITE,MAP_SHARED,shared_mem,0);
	if(gmp == MAP_FAILED)
	{
		perror("Mmap goldmine");
		sem_post(SEM);
		clear(1);
	}
	gmp->rows = this->rows;
	gmp->cols = this->cols;	
	//gmp->players[0] = this->currpid;
	for(unsigned i = 0; i<5; i++)
	{
		gmp->players[i] = 0;
	}
	gmp->daemon_ID = this->currpid;
	for(unsigned i =0 ; i<cols*rows; i++)
  	{
  		gmp->map[i] = daemon_map[i];
  	}
  	
  	
  	char incoming_byte;
  	READ(new_sockfd, &incoming_byte, 1);
	socket_player_read(incoming_byte);
	
	sem_post(SEM);
	//socket_player_write();
	socket_comm_loop();
}

void Game::daemon_init()
{
	if(fork()>0)
	{
		exit(0);
	}
	if(setsid()==1)
	{
		exit(1);
	}
	for(int i=0;i<sysconf(_SC_OPEN_MAX);++i)
	{
		close(i);
	}
	open("/dev/null",O_RDWR);
	open("/dev/null",O_RDWR);
	open("/dev/null",O_RDWR);
	//open("/home/jait/FIFO",O_RDWR);
	//std::cerr<<"Daemon Process ID=" <<getpid() << "\n";
	umask(0);
	chdir("/");
	for(unsigned i;i<5;i++)
	{
		daemon_readqfd_list[i] = 0;
	}
	
}

void Game::socket_comm_loop()
{
	unsigned char incoming_byte;
    int num_read;
    short n;
    
    while((num_read=READ(new_sockfd, &incoming_byte, 1))==num_read)
    {
    	std::cerr<<".."<<std::endl;
      if(num_read==-1 && errno==EINTR)
      { //interrupted by signal
        continue; 
      }
      else if((incoming_byte & G_SOCKPLR) == G_SOCKPLR )
      {
      	std::cerr<<"SOCK"<<std::endl;
      	socket_player_read(incoming_byte);
      }
      else if((incoming_byte & G_SOCKMSG) == G_SOCKMSG )
      {
      	std::cerr<<"MSG"<<std::endl;
      	socket_txt_read(incoming_byte);
      }
      else if(incoming_byte == 0)
      {
      	std::cerr<<"MAP"<<std::endl;
      	socket_map_read();
      }
    }
}

void Game::socket_player_write()
{
	std::cerr<<"BEf SPW Players::"<<getnum_players()<<std::endl;
	char plyr = 0;
	plyr |= G_SOCKPLR;
	for(unsigned i = 0; i<5; i++)
	{
		if(gmp->players[i] != 0)
		{
			plyr |= plrbits[i];
		}
	}
	if(getnum_players() > 0)
	{
		WRITE(new_sockfd,&plyr,1);
  	}
	else
	{
		std::cerr<<"NO PLYR"<<std::endl;
		WRITE(new_sockfd,&plyr,1);
		sem_close(SEM);
		close(shared_mem);
		sem_unlink("/gold");
		shm_unlink("/goldmemory");
		exit(0);
	}
	std::cerr<<"Aft SPW Players::"<<getnum_players()<<std::endl;
}

void Game::socket_player_read(char incoming_byte)
{
	std::cerr<<"BEf SPR Players::"<<getnum_players()<<std::endl;

	for(unsigned int i = 0 ;i<5;i++)
      	{
      		if((incoming_byte & plrbits[i]) == plrbits[i])
      		{
      			if(gmp->players[i] ==0)
      			{
      				gmp->players[i] = gmp->daemon_ID;
      				std::string mq_name = filenames[i];
      				struct mq_attr mq_attributes;
  					mq_attributes.mq_flags=0;
  					mq_attributes.mq_maxmsg=10;
  					mq_attributes.mq_msgsize=120;
      				if((daemon_readqfd_list[i]=mq_open(mq_name.c_str(), O_RDONLY|O_CREAT|O_EXCL|O_NONBLOCK,S_IRUSR|S_IWUSR, &mq_attributes))==-1)
  						{
    						perror("mq_open");
    						exit(1);
  						}			
  					struct sigevent mq_notification_event;
  	mq_notification_event.sigev_notify=SIGEV_SIGNAL;
  	mq_notification_event.sigev_signo=SIGUSR2;
  	mq_notify(daemon_readqfd_list[i], &mq_notification_event);		
      			}
      		}
      		else
      		{
      			if(gmp->players[i] !=0)
      			{
      				std::cerr<<"closing "<<i<<std::endl;
      				std::string mq_name = filenames[i];
      				gmp->players[i] = 0;
      				mq_close(daemon_readqfd_list[i]);
  					mq_unlink(mq_name.c_str());
  					if(getnum_players() == 0)
					{
      					sem_close(SEM);
						close(shared_mem);
						sem_unlink("/gold");
						shm_unlink("/goldmemory");
						exit(0);
					}
      			}
      		}
      	}
     
}

void Game::socket_map_write()
{
	std::cerr<<"WR MAP"<<std::endl;
	std::vector<std::pair<short,unsigned char>> sendpair;
	std::pair<short,unsigned char> mapbit;
	unsigned map_changes = 0;
	
	for(short i = 0; i < rows*cols; i++)
	{
		if(gmp->map[i] != daemon_map[i])
		{
			map_changes++;
			mapbit.first = i;
			mapbit.second = gmp->map[i];
			sendpair.push_back(mapbit);
			daemon_map[i] = gmp->map[i];
		}
	}
	if(map_changes > 0)
	{
		unsigned char protocolbyte = 0;
		WRITE(new_sockfd,&protocolbyte,sizeof(protocolbyte));
		WRITE(new_sockfd,&map_changes,sizeof(map_changes));
		for(short i = 0; i < map_changes; i++)
		{
			short offset = sendpair.at(i).first;
			unsigned char mapchar = sendpair.at(i).second;
			WRITE(new_sockfd,&offset,sizeof(offset));
			WRITE(new_sockfd,&mapchar,sizeof(mapchar));
		}
	}
}

void Game::socket_map_read()
{
	unsigned map_changes;
	READ(new_sockfd, &map_changes, sizeof(map_changes));
	std::cerr<<"Processing "<<map_changes<<std::endl;
	for(short i = 0; i < map_changes; i++)
	{
		short offset;
		unsigned char mapchar;
		READ(new_sockfd,&offset,sizeof(offset));
		READ(new_sockfd,&mapchar,sizeof(mapchar));
		gmp->map[offset] = mapchar;
		daemon_map[offset] = mapchar;
	}
	sendsig1();
}

void Game::daemon_sigsetup()
{
	struct sigaction plyrchange;
  	plyrchange.sa_handler=player_changer; 
  	sigemptyset(&plyrchange.sa_mask); 
  	sigaddset(&plyrchange.sa_mask,SIGUSR1);
  	plyrchange.sa_flags=0;
  	sigaction(SIGHUP, &plyrchange, nullptr); 
  	
	struct sigaction mapupdate;
  	mapupdate.sa_handler=mapper; 
  	sigemptyset(&mapupdate.sa_mask); 
  	mapupdate.sa_flags=0;
  	sigaction(SIGUSR1, &mapupdate, nullptr); 
	
	
  	struct sigaction dmnmessage;
  	dmnmessage.sa_handler=msgreader; 
  	sigemptyset(&dmnmessage.sa_mask); 
  	dmnmessage.sa_flags=0;
  	sigaction(SIGUSR2, &dmnmessage, nullptr); 
  
}

void Game::player_changer(int)
{
	me->socket_player_write();
}

void Game::mapper(int)
{
	me->socket_map_write();
}

void Game::msgreader(int)
{
	me->socket_txt_write();
}

void Game::socket_txt_write()
{
	unsigned index;
	int err;
	char plyr = 0;
	char msg[121];
  	memset(msg, 0, 121);
  	std::string sendmsg;
  	short length;
  	

	for(unsigned i = 0; i<5; i++)
	{
		if(gmp->players[i] == gmp->daemon_ID)
		{
			struct sigevent mq_notification_event;
  			mq_notification_event.sigev_notify=SIGEV_SIGNAL;
  			mq_notification_event.sigev_signo=SIGUSR2;
  			
  			mq_notify(daemon_readqfd_list[i], &mq_notification_event);
  			
  			
  			if((err=mq_receive(daemon_readqfd_list[i], msg, 120, nullptr))!=-1)
  			{
    			sendmsg=msg;
    			plyr |= G_SOCKMSG;
				plyr |= plrbits[i];
    			memset(msg, 0, 121);//set all characters to '\0'
  			}
  			
			
			WRITE(new_sockfd,&plyr,1);
			length = short(sendmsg.length());
  			WRITE(new_sockfd,&length,sizeof(length));
  			for(short i = 0 ;i <length; i++)
  			{
  				char send = sendmsg[i];
  				WRITE(new_sockfd,&send,1);
  			}
		}
	}
  	
}

void Game::socket_txt_read(char incoming_byte)
{
	short length;
	std::string msg = "";
	char recieve;
	READ(new_sockfd,&length,sizeof(length));
	for(short i = 0 ;i <length; i++)
  	{
  		READ(new_sockfd,&recieve,1);
  		msg +=recieve;
  	}
	for(unsigned int i = 0 ;i<5;i++)
    {
      	if((incoming_byte & plrbits[i]) == plrbits[i])
      	{
      		write_message(msg,filenames[i]);
      	}
    }
    //get num bytes of msg
    //get msg in string
    // write msg to each mq
}


