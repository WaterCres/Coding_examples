import pandas as pd
import string
import logging
import csv
import os
import json
import adsigno

from datetime import datetime
from tablib import Dataset


from django.conf import settings

from .models import *

logger = logging.getLogger('file.errors')

def index_to_dict(indexes):
    idx = {}
    for pair in indexes:
        # advisor and/or email was not assigned
        if pair == '':
            continue
        key,value=pair.split(';')
        idx[key]=value
    return idx

def load_file(file):
    try:
        exceldata = pd.ExcelFile(file)
        df = pd.read_excel(exceldata,sheet_name=None)
        dataset = {}
        for sheet in df.keys():
            dataset[sheet] = Dataset().load(df[sheet])
    except ValueError as e:
        logger.error("\n%s\nWrong file format uploaded, %s\n%s" % (datetime.now(),file,e))
        return (None,-1)
    return (dataset,0)


def imp_stds(dataset,index):

    count = 0
    try:

        for grp in set(dataset[index['Group id']]):
            new_group = Group(id=grp)
            new_group.save()

        for key,value in index.items():
            index[key] = dataset.headers.index(value)

        for student in dataset:
            if User.objects.filter(u_name=student[index['Username']]).exists():
                continue
            else:
                count += 1
                new_user = User( 
                                u_name=student[index['Username']],
                                full_name=student[index['Full name']],
                                email=student[index['Email']]
                                )
                new_user.save()

                new_group = Group.objects.get(id=student[index['Group id']])
                new_group.prefs=student[index['Preferences']]
                new_group.last_sub=student[index['Submission time']]
                new_group.save()


                new_student = Student(
                                user=new_user,
                                std_type=student[index['Type']],
                                group=new_group
                                )
                new_student.save()
    
    except (KeyError,ValueError) as e:
        # file not formatted right
        logger.error("\n%s\nFile was not formatted right, or headers were assigned wrong\n%s" % (datetime.now(),e))
        return -1
    
    logger.info("\n%s\nUsers has been imported from file" % (datetime.now()))
    return count

def imp_tops(dataset,index):

    try:
        import_user = User.objects.get(u_name="Import")
    except Exception as e:
        # import user was not found
        logger.error("\n%s\nImport user has been deleted\n%s" % (datetime.now(),e))
        return -2
    try:
        for key,value in index.items():
            index[key] = dataset.headers.index(value)

        imported_topics = {}
        alph = list(string.ascii_lowercase)
    
        for topic in dataset:
            # if the topic has been seen before in this import
            if topic[index['Title']] in imported_topics.keys():
                imported_topics[topic[index['Title']]] +=1
            # if the topic has previously been imported
            elif Topic.objects.filter(title=topic[index['Title']]).exists():
                continue
            # otherwise import a new topic
            else:
                imported_topics[topic[index['Title']]] =1
                

                new_topic = Topic (
                                    title = topic[index['Title']],
                                    num = topic[index['id']],
                                    cap_min = topic[index['Capacity min']],
                                    cap_max = topic[index['Capacity max']],
                                    origin = import_user,
                                    requirements = topic[index['Minikurser']],
                                    institute = topic[index['Institute']],
                                    inst_short = topic[index['Institute short']],
                                    available_to = topic[index['Available to']],
                                    location = topic[index['Location']]
                                    )
                try:
                    new_topic.advisor_main = topic[index['advisor']]
                except (KeyError,IndexError):
                    new_topic.advisor_main = "no_teacher"

                try:
                    new_topic.email = topic[index['email']]
                except (KeyError,IndexError):
                    new_topic.email = "no_teacher"

                new_topic.save()

        # for each key in imported_topics
        # create value number of teams
        for name in imported_topics.keys():
            for team in range(imported_topics[name]):

                topic = Topic.objects.get(title=name)

                new_team = Team (
                                topic = topic,
                                key = topic.key+alph[team]
                                )
                new_team.save()
    except (KeyError,IndexError,ValueError,AttributeError) as e:
        logger.exception("\n%s\nSuspected wrong file, or wrong header assignment.\n%s" % (datetime.now(),e))
        return -1

    if imported_topics == {}:
        return 1
    else:
        logger.info("\n%s\nTopics has been imported from file" % (datetime.now()))
        return 0

# get the different availabilities, currently in the db
def get_types():
    topics = Topic.objects.all()
    return list(set([topic.available_to for topic in topics]))
    
# get the types of students currently in the database
def get_std_types():
    studs = Student.objects.all()
    return list(set([stud.std_type for stud in studs]))

# write students to csv file
def students_to_csv(students,path):
    header = ['grp_id','username','type','priority_list','student_id','full_name','email','timestamp']
    with open(path+'/students.csv','w',encoding='UTF8',newline='') as std:
        writer = csv.writer(std,delimiter=';')

        writer.writerow(header)
        # each row has the format:
        # grp_id;username;type;priority;student_identifier;student_full_name;student_email;registration_time
        for st in students:
            data = [str(st.group.id),
                    str(st.user.u_name),
                    str(st.std_type),
                    str(st.get_prefs()),
                    str(st.user.id),
                    str(st.user.full_name),
                    str(st.user.email),
                    str(st.group.last_sub)]
            writer.writerow(data)
    return

# write topics to csv file
def topics_to_csv(teams,path):
    header = ['ID','team','title','min_cap','max_cap','type','prj_id','instit','institute','mini','wl','teachers','email']
    with open(path+'/projects.csv','w',encoding='UTF8',newline='') as tops:
        writer = csv.writer(tops,delimiter=';')

        writer.writerow(header)
        # each row contains a topic and has the format:
        # id;team;title;min_capacity;max_capacity;type;proj_identifier;institute_abbreviation;institute_name;needs;location
        for team in teams:
            data =[str(team.topic.num),
                   str(team.key)[-1],
                   str(team.topic.title),
                   str(team.topic.cap_min),
                   str(team.topic.cap_max),
                   str(team.topic.available_to),
                   str(team.topic.num)+(str(team.key)[-1]),
                   str(team.topic.inst_short),
                   str(team.topic.institute),
                   str(team.topic.requirements),
                   str(team.topic.location),
                   str(team.topic.advisor_main),
                   str(team.topic.email),
                   ]
            writer.writerow(data)
    return

# write restrictions to csv file
def restrict_to_csv(res,path):
    output = {'nteams':[]}
    for adv,value in res.items():
        topics = Topic.objects.filter(advisor_main=adv)
        output['nteams'].append(
            {"username":adv,
            "groups_max":value,
            "groups_min":0,
            "capacity_max":value*sum([top.cap_max for top in topics]),
            "capacity_min":0,
            "topics":[top.num for top in topics]})

    with open (path+'/restrictions.json','w',) as out:
        json.dump(output,out)

    return

# write compatible types to csv file
def types_to_csv(compat,path):
    with open(path+'/types.csv','w',encoding='UTF8',newline='') as types:
        writer = csv.writer(types,delimiter=';')
        # data is provided as a dict
        for key,value in compat.items():
            data = [str(key)]+value
            writer.writerow(data)
    return

# create the files needed for the solver, and start computation
def prepare_solver(selected, restrictions, compatibilities,scenario ):
    # make a new directory for the scenario
    path = settings.MEDIA_ROOT + scenario
    os.makedirs(path,exist_ok=True)

    topics = Topic.objects.all()
    # if "alle" is not in the selected, exclude the topics not chosen
    # the topics available to "alle" should still be included though
    if "alle" not in selected:
        temp = []
        for topic in topics:
            if any(elem in topic.available_to for elem in selected+["alle"]):
                temp.append(topic)
        topics = temp
    
    # get the teams associated with the topics
    teams = Team.objects.filter(topic__in=topics)

    # get the default restrictions
    res = {}
    for team in teams:
        advi = str(team.topic.advisor_main)
        if advi in res.keys():
            res[advi] += 1
        else:
            res[advi] = 1
    
    for item in restrictions:
        if item == "":
            continue
        tmp = item.split(':')
        if tmp[0] in res.keys():
            res[tmp[0]]=tmp[1]


    # if "alle" is not in selected get only the students who match the selected type
    if "alle" not in selected:
        students = Student.objects.filter(std_type__in=selected)
    # otherwise get all students
    else:
        students = Student.objects.all()

    # write students to csv file
    students_to_csv(students,str(path))
    # write topics to csv file
    topics_to_csv(teams,str(path))


    # write restrictions to csv files
    restrict_to_csv(res,str(path))

    # write compabilities to csv
    types_to_csv(compatibilities, str(path))
    

    
    return path