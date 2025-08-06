import adsigno.solve as solve

from django.contrib.auth.decorators import login_required
from django.contrib.auth import logout
from django.contrib import messages
from django.shortcuts import render, redirect
from django.conf import settings
from datetime import datetime
import os
import json

from .utils import AttributeDict
from .models import *
from .processing import *




def index(request):
	
	context	= {
		'sitename':'Home',
		'admin_email':'aaskh20@student.sdu.dk',
		'admin_name':'aaskh20',
	}

	return render(request,'home.html',context)

@login_required(login_url='/microsoft/to-auth-redirect/')
def login(request):
	# login using sdu sso, then redirect to front page
	messages.info(request,"Login successful, welcome "+str(request.user))
	return redirect('index')

def logout_user(request):
	logout(request)
	messages.info(request,"Logout successful")
	return redirect('index')


@login_required(login_url='/microsoft/to-auth-redirect/')
def submit(request):
	# A wild user without permissions appear
	if (not request.user.is_staff) and (request.user not in [t.user.u_name for t in Teacher.objects.all()]):
		return redirect('index')
	
	# sitename
	context = {'sitename':'Data'}

	# not importing
	if request.method != 'POST':
		return render(request,'submit.html',context)
	
	# import uploaded file
	if 'file' in request.FILES:

		upl_file = request.FILES['file']
		new_doc = Document(name=upl_file.name,
		     				file=upl_file)
		new_doc.save()
		dataset,resp = load_file(new_doc.file)
		# file was successfully read
		if resp == 0:
			context["stud"] = [ "Username",
		      					"Full name",
								"Email",
								"Type",
								"Group id",
								"Preferences",
								"Submission time"
								]
			context["topics"] = [ "Title",
								  "id",
								  "Capacity max",
								  "Capacity min",
								  "Available to",
								  "Minikurser",
								  "Institute",
								  "Institute short",
								  "Location"
								  ]
			context["f_name"] = new_doc.name.lower()
			headers ={}
			for name,sheet in dataset.items():
				headers[name]=sheet.headers
			# headers = [sheet.headers for sheet in dataset.values()]

			context["headers"] = headers
			context["sheets"] = (len(headers),list(headers.keys()))

			# pass the file on to the next view
			request.session["data_file"] = new_doc.id

			return render(request,'submit.html',context)

		else:
			messages.error(request,"File not recognized, make sure it is in '.xlsx' format")
			
	return redirect(request.META['HTTP_REFERER'])

# intermediate view which handles imports and returns the user to the submit page
def impot(request):
	# should only be accessible when posting
	if request.method != 'POST':
		return redirect(request.META['HTTP_REFERER'])
	
	file = Document.objects.get(id=request.session["data_file"])
	dataset,_ = load_file(file.file)

	content = request.POST.getlist('contents')

	print(content)

	sheet_idx = {}
	for pair in content:
		key,value = pair.split(';')
		sheet_idx[key] = value

	if 'students' in sheet_idx.keys():
		indexes = index_to_dict(request.POST.getlist('index_stud'))
		count = imp_stds(dataset[sheet_idx['students']],indexes)

	# post a message showing the successfulness
		if count <0:
			messages.error(request,"Headers were not set right")
		elif bool(count):
			messages.success(request,str(count)+" Students successfully imported")
		else:
			messages.info(request,"All students are already imported")

	
	if 'topics' in sheet_idx.keys():
		indexes = index_to_dict(request.POST.getlist('index_top'))
		resp = imp_tops(dataset[sheet_idx['topics']],indexes)
	
	# post a message showing the successfulness
		if resp == 1:
			messages.info(request,"All topics already Present")
		elif resp == -1:
			messages.error(request,"Headers were not set right")
		elif resp == 0:			
			messages.success(request,"Topics successfully imported")
		else:
			messages.error(request,"An error happened, please contact your friendly admin")

	# cleanup before exiting
	if len(file.file) > 0:
		os.remove(file.file.path)
	file.delete()

	return redirect(request.META['HTTP_REFERER'])

@login_required(login_url='/microsoft/to-auth-redirect/')
def assign(request):
	# A wild user without permissions appear
	if not request.user.is_staff:
		return redirect('index')
	
	types_topics = get_types()
	types_students = get_std_types()

	
	context = {
		'sitename':'Assignment',
		'types': types_topics,
		'students':types_students,
	}
	if request.method != 'POST':
		return render(request,'assignment.html',context)
	else:
		# gather all the information entered on the assignment page
		selected = request.POST.getlist('selected')
		restrictions = request.POST.getlist('restrictions')

		scenario = request.POST['scenario']
		
		if scenario == "":
			scenario = datetime.now().strftime("%d%m%Y_%H%M")

		compatibilities = {}
		for t in types_students:
			temp = request.POST.getlist(t)
			if temp == []:
				continue
			compatibilities[t]=temp
		
		# write data to csv files
		path = prepare_solver(selected,restrictions,compatibilities,scenario)

		options = AttributeDict()
		options.allsol = 'alsol' in request.POST
		options.instability = 'instab' in request.POST
		options.expand_topics = 'expand' in request.POST
		options.groups = request.POST['group']
		options.Wmethod = request.POST['weight']
		options.cut_off = int(request.POST['cut'])
		options.prioritize_all = 'prio_all' in request.POST
		options.allow_unassigned = 'allow_un' in request.POST
		options.min_preferences = int(request.POST['min_prefs'])
		options.solution_file = path+'/solution.txt'

		if request.POST['cut_type'] == 'None':
			options.cut_off_type = None
		else:
			options.cut_off_type = request.POST['cut_type']

		# start the solver
		try:
			solve.solve(path,options)
			Solution( title = scenario,
					  stats = path+"/sln/stat_001.json",
					  sln = path+"/sln/sol_001.txt"
					  ).save()
			# when success transfer to result page
			messages.info(request,"results for %s can be seen at the results page" % scenario)
		except SystemError as e:
			messages.error(request,e)

		return redirect(request.META['HTTP_REFERER'])

# the view visualizing the results of the computation
@login_required(login_url='/microsoft/to-auth-redirect/')
def results(request):

	context = {
		'sitename':'Results',
		'options':[s.title for s in Solution.objects.all()],
		'show':False
	}
	# let the user choose which scenario they want to view
	# when a scenario has been chosen retrieve the data
	if request.method == 'POST':
		solution = Solution.objects.filter(title = request.POST['scenario'])[0]
		# try:
		with open(solution.stats,'r') as json_file:
			stats = json.load(json_file)
		assignments = []
		with open(solution.sln,"r") as sln:
			for line in sln:
				tmp = line.strip().split("\t")
				top = Topic.objects.get(num=tmp[1])
				assignments += [{ 'topic':top,
									'team':Team.objects.get(key=top.key+tmp[2]),
									'student':User.objects.get(u_name__iexact=tmp[0]),
								}]
	
		# remove 0 values from priority stats
		stats['priority'] = { key:value for key,value in stats['priority'].items() if value != 0 }

		tmp = {}	
		for entry in assignments:
			if entry['team'] in tmp.keys():
				tmp[entry['team']] += ", "+entry['student'].u_name
			else:
				tmp[entry['team']] = entry['student'].u_name

		# data for the different tables on the page
		# table showing the distribution of groups
		context['table'] = [{'topic':team.topic.title,
							'team':team.key[3:],
							'students':students} for team,students in tmp.items()]
		
		# table showing the distribution of students
		context['table_expanded'] = [{'topic':entry['topic'].title,
									'team':entry['team'].key[3:],
									'students':entry['student'].u_name,
									'priorities':entry['student'].student.group.prefs.replace(",",", ")} for entry in assignments]

		# table showing the popularity of the topics
		groups = Group.objects.all()
		# list all the 1st, 2nd ... priorities
		pop_dict = {}
		for groop in groups:
			tmp = groop.prefs.split(",")
			for i in range(len(tmp)):
				s = '%s. priority' % (i+1)
				if s in pop_dict.keys():
					pop_dict[s] += [tmp[i]]
				else:
					pop_dict[s] = [tmp[i]]

		# for every priority count how many times a topic shows up
		topic_pop_dict = {}
		for prio,tops in pop_dict.items():
			for top in tops:
				top = str(int(top))
				if top in topic_pop_dict.keys():
					if prio in topic_pop_dict[top].keys():
						topic_pop_dict[top][prio] += 1
					else:
						topic_pop_dict[top].update({prio:1})
				else:
					topic_pop_dict[top] = { 'topic':Topic.objects.get(num=top),
											prio:1}
		# find the worst priority of any topic
		high = 0
		for entry in topic_pop_dict.values():				
			tmp = 0
			for k in entry.keys():
				if k == 'topic':
					pass
				else:
					tmp = max(tmp,int(k.split('.')[0]))
			high = max(tmp,high)

		# update all entries to have the same number of priorities
		for topic in topic_pop_dict.keys():
			for i in range(high):
				idx = str(i+1) +". priority"
				if idx not in topic_pop_dict[topic].keys():
					topic_pop_dict[topic].update({idx:0})
		
		# collect data for table
		pop = [{	'id':int(num),
					'title':data['topic'].title,
					'type':data['topic'].available_to,
					'institute':data['topic'].inst_short,
					'total':sum([count for key,count in data.items() if key != 'topic' ]),
					} for num,data in topic_pop_dict.items() ]
		
		for entry in pop:
			entry.update({prio:value for prio,value in topic_pop_dict[str(entry['id'])].items() if prio != 'topic'})

		context['popularity'] = pop
		context['stats'] = stats
		context['title'] = solution.title
		context['show'] = True

	return render(request,'visualize.html',context)



