from front.models import Teacher

def teachers(request):
    teachers = []
    for t in Teacher.objects.all():
        teachers.append(t.user.u_name)
    return{'teachers':teachers}