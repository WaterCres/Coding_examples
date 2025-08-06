from random import randrange

from django.db import models

class Document(models.Model):
	name = models.CharField(max_length=100, null=True, blank=True)
	file = models.FileField()

class Group(models.Model):
	"""
	A Group contains one or more Students
	used for preferences
	and assignments
	"""
	id = models.PositiveIntegerField(primary_key=True)
	prefs = models.TextField(default="",blank=True)
	last_sub = models.TextField(default="")

	def __str__(self):
		return str(','.join([users.user.u_name for users in self.belongs.all()]))

	@property
	def get_members(self):
		return self.belongs.all()



class User(models.Model):
	"""The base for all users"""
	u_name = models.CharField(max_length=10 ,default="")
	full_name = models.TextField(default="")
	email = models.EmailField(default="")

	def __str__(self):
		return self.u_name
		

class Student(models.Model):
	"""A user who is a student"""
	user = models.OneToOneField(User, on_delete=models.CASCADE, primary_key=True)
	std_type = models.CharField(max_length=50 ,default="")
	group = models.ForeignKey(Group, on_delete=models.SET_NULL, null=True, related_name='belongs')

	def __str__(self):
		return str(self.user)
	
	def get_prefs(self):
		return self.group.prefs

class Teacher(models.Model):
	"""
	A user who is a Teacher
	capacity = 0 means no restrictions
	"""
	user = models.OneToOneField(User, on_delete=models.CASCADE, primary_key=True)
	teams_max = models.PositiveIntegerField(default=0)
	teams_min = models.PositiveIntegerField(default=0)
	students_max = models.PositiveIntegerField(default=0)
	students_min = models.PositiveIntegerField(default=0)
	admin = models.BooleanField(default=False)

	def __str__(self):
		return str(self.user.u_name)

# Key generation found on
# https://github.com/workmajj/django-unique-random

CHARSET = '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ'
LENGTH = 3
MAX_TRIES = 32

class Topic(models.Model):
	"""
	Topics available to students
	"""
	key = models.CharField(max_length=LENGTH, editable=False, unique=True, primary_key=True)
	num = models.PositiveIntegerField(default=0)
	title = models.CharField(default="",max_length=200)
	description = models.TextField(default="")
	advisor_main = models.TextField(default="")
	advisor_co = models.TextField(default="")
	email = models.TextField(default="")
	cap_max = models.PositiveIntegerField(default=0)
	cap_min = models.PositiveIntegerField(default=0)
	origin = models.ForeignKey(User, on_delete=models.CASCADE)
	location = models.TextField(default="", blank=True, null=False)
	requirements = models.TextField(default="",blank=True)
	institute = models.TextField(default="")
	inst_short = models.CharField(default="",max_length=10)
	available_to = models.TextField(default="")
	hidden = models.BooleanField(default=False)


	class Meta:
		verbose_name = "Topic"
		verbose_name_plural = "Topics"

	def __str__(self):
		return "%s: %s" % (self.key, self.title)

	def save(self, *args, **kwargs):
		"""
		Upon saving, generate a code by randomly picking LENGTH number of
		characters from CHARSET and concatenating them. If code has already
		been used, repeat until a unique code is found, or fail after trying
		MAX_TRIES number of times. (This will work reliably for even modest
		values of LENGTH and MAX_TRIES, but do check for the exception.)
		Discussion of method: http://stackoverflow.com/questions/2076838/
		"""
		loop_num = 0
		unique = False
		while not unique:
			if loop_num < MAX_TRIES:
				new_key = ''
				for i in range(LENGTH):
					new_key += CHARSET[randrange(0, len(CHARSET))]
				if not Topic.objects.filter(key=new_key):
					self.key = new_key
					unique = True
				loop_num += 1
			else:
				raise ValueError("Couldn't generate a unique code.")
		super(Topic, self).save(*args, **kwargs)

class Team(models.Model):
	"""Teams for each topic"""
	topic = models.ForeignKey(Topic, on_delete=models.CASCADE)
	key = models.CharField(max_length=5 ,default="", primary_key=True)

	def __str__(self):
		return self.key


class Solution(models.Model):
	"""
	The solution(s) to a scenario
	"""
	title = models.TextField(default="")
	stats = models.FilePathField()
	sln = models.FilePathField()


	def __str__(self):
		return self.title