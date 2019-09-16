import numpy as np
import utils
import random


class Agent:
    
    def __init__(self, actions, Ne, C, gamma):
        self.actions = actions
        self.Ne = Ne # used in exploration function
        self.C = C
        self.gamma = gamma
        # self.di = {0:3,1:2,2:1,3:0}
        # Create the Q and N Table to work with
        self.Q = utils.create_q_table()
        self.N = utils.create_q_table()
        self.points = 0
        self.s = None
        self.a = None
        self._train = True

    def train(self):
        
        #Q learning
        # sample_val = r + self.gamma*max(self.Q[Next_state]) # state transition(s,a,r,s')
        # alpha = self.C / (self.C + self.N[state]
        # self.Q[state] = (1-alpha)*self.Q[state] + alpha*(r +self.gamma*max(explore_function))
        self._train = True

 

    def eval(self):
        self._train = False

    # At the end of training save the trained model
    def save_model(self,model_path):
        utils.save(model_path, self.Q)

    # Load the trained model for evaluation
    def load_model(self,model_path):
        self.Q = utils.load(model_path)

    def reset(self):
        # print("reset here")
        self.points = 0
        self.s = None
        self.a = None

    def act(self, state, points, dead):
        '''
        :param state: a list of [snake_head_x, snake_head_y, snake_body, food_x, food_y] from environment.
        :param points: float, the current points from environment
        :param dead: boolean, if the snake is dead
        :return: the index of action. 0,1,2,3 indicates up,down,left,right separately

        TODO: write your function here.
        Return the index of action the snake needs to take, according to the state and points known from environment.
        Tips: you need to discretize the state to the state space defined on the webpage first.
        (Note that [adjoining_wall_x=0, adjoining_wall_y=0] is also the case when snake runs out of the 480x480 board)

        '''
        # state = [adjoining_wall_x, adjoining_wall_x, food_dir_x, food_dir_y,adj_body_u,adj_body_b,adj_body_l,adj_body_r,a]
        #decode state
        # print(state)
        if(state[0] == 40):
            adjoining_wall_x = 1
        elif(state[0] == 560-80):
            adjoining_wall_x = 2
        else:
            adjoining_wall_x = 0
        
        if(state[1] == 40):
            adjoining_wall_y = 1
        elif(state[1] == 560-80):
            adjoining_wall_y = 2
        else:
            adjoining_wall_y = 0
        
        if(state[3] == state[0]): # same x coord
            food_dir_x = 0
        elif(state[3] < state[0]): # food on head left
            food_dir_x = 1
        else:
            food_dir_x = 2

        if(state[4] == state[1]): # same y coord
            food_dir_y = 0
        elif(state[4] < state[1]): # food on head left
            food_dir_y = 1
        else:
            food_dir_y = 2

        temp_up = (state[0], state[1] - 40)
        temp_down = (state[0], state[1] +40)
        temp_left = (state[0] - 40, state[1])
        temp_right = (state[0] + 40, state[1])

        if(temp_up in state[2]): # up
            adj_body_u = 1
        else:
            adj_body_u = 0
        if(temp_down in state[2]): #down
            adj_body_b = 1
        else:
            adj_body_b = 0
        if(temp_left in state[2]): #left
            adj_body_l = 1
        else:
            adj_body_l = 0
        if(temp_right in state[2]): #right
            adj_body_r = 1
        else:
            adj_body_r = 0
        idx = [[adjoining_wall_x],[adjoining_wall_y],[food_dir_x],[food_dir_y],[adj_body_u],[adj_body_b],[adj_body_l],[adj_body_r]]
        if(self._train):#training process
            if(points == self.points and not dead):
                reward = -0.1
            elif(points > self.points):
                reward = 1
            elif(dead):
                reward = -1
            
            temp_N = self.N[idx].reshape((4,)) #explore frequency count
            temp_Q = self.Q[idx].reshape((4,))
            func = np.zeros(4)
                # print(temp_Q.shape)
            func[0] = self.explore_function(temp_Q[0],temp_N[0])
            func[1] = self.explore_function(temp_Q[1],temp_N[1])
            func[2] = self.explore_function(temp_Q[2],temp_N[2])
            func[3] = self.explore_function(temp_Q[3],temp_N[3])
            # if(dead):
            #     # print("dead")
            #     a = self.a[0] # previous action
            # else:
            t_a = np.argsort(func)[::-1]
            # a = self.di[ta]
            a = t_a[0]

            # t_a = np.argsort(func)[::-1]
            # a = t_a[0] # explore action
        # print(type(self.actions[0]))
            #update Q and N
            new_idx = idx + [a]
            if(self.s != None):
                # prev_a = self.di[self.a]
                prev_idx = self.s + self.a#[prev_a]
                prev_N = self.N[prev_idx]
                # prev_Q = self.Q[self.s]
                alpha = self.C / (self.C + prev_N)
                self.Q[prev_idx] += alpha*(reward + self.gamma*np.amax(temp_Q) - self.Q[prev_idx])
                #(1-alpha)*self.Q[prev_idx] + alpha*(reward + self.gamma*np.amax(temp_Q))
            if(dead):
                self.reset()
            else:
                self.s = idx
                self.points = points
                self.a = [a]
                self.N[new_idx] += 1 # update N
        else:# solve MDP
            # print("here")
            temp_Q = self.Q[idx].reshape((4,))
            # a = self.di[ta]
            t_a = np.argsort(temp_Q)[::-1]
            # a = self.di[ta]
            a = t_a[0]
            # print(a)
        return a

    def explore_function(self,u,n):
        # print("here")
        # print(u)
        if n < self.Ne:
            # print("here")
            return 1
        else:
            return u